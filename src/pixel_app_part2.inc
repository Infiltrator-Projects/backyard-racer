            }
        }
        make_tone(pale_, rgba(239, 231, 207));
        make_tone(gold_, rgba(242, 211, 154));
        make_tone(red_, rgba(236, 106, 86));
        make_tone(dark_, rgba(43, 39, 32));
    }

    enum class Tone { Pale, Gold, Red, Dark };

    void draw(InfiltratrSurface& destination, int x, int y,
              const std::string& value, Tone tone, int scale = 2) const {
        const InfiltratrSurface& atlas = surface_for(tone).get();
        int cursor = x;
        for (char raw : value) {
            char c = raw;
            if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
            std::size_t index = characters_.find(c);
            if (index == std::string::npos) index = characters_.find('?');
            infiltratr_surface_blit_region_scaled_nearest(
                &destination, &atlas,
                static_cast<int>(index) * cell_w_, 0, cell_w_, cell_h_,
                cursor, y, cell_w_ * scale, cell_h_ * scale);
            cursor += cell_w_ * scale;
        }
    }

    int width(const std::string& value, int scale = 2) const {
        return static_cast<int>(value.size()) * cell_w_ * scale;
    }

private:
    const std::string characters_ =
        " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$-/:.%+?!(),";
    static constexpr int cell_w_ = 6;
    static constexpr int cell_h_ = 8;
    SurfaceOwner base_;
    SurfaceOwner pale_;
    SurfaceOwner gold_;
    SurfaceOwner red_;
    SurfaceOwner dark_;

    void make_tone(SurfaceOwner& destination, Color tint) {
        if (!infiltratr_surface_copy_luma_tinted(&destination.get(), &base_.get(), tint)) {
            throw std::bad_alloc();
        }
    }

    const SurfaceOwner& surface_for(Tone tone) const {
        switch (tone) {
            case Tone::Gold: return gold_;
            case Tone::Red: return red_;
            case Tone::Dark: return dark_;
            case Tone::Pale: default: return pale_;
        }
    }
};

struct CarArt {
    std::string id;
    SurfaceOwner source;
    SurfaceOwner body;
    SurfaceOwner details;
    SurfaceOwner rear_wheel;
    SurfaceOwner front_wheel;
    double rear_x = 0.23;
    double front_x = 0.77;
    double wheel_y = 0.80;
    double wheel_r = 0.105;
    int paint_min = 80;
    int paint_max = 245;
};

void split_car_art(CarArt& art) {
    const int width = art.source.width();
    const int height = art.source.height();
    art.body.resize(width, height);
    art.details.resize(width, height);
    art.body.clear(rgba(0, 0, 0, 0));
    art.details.clear(rgba(0, 0, 0, 0));

    const int source_radius = std::max(2, static_cast<int>(std::lround(height * art.wheel_r)));
    const int wheel_size = source_radius * 2 + 1;
    art.rear_wheel.resize(wheel_size, wheel_size);
    art.front_wheel.resize(wheel_size, wheel_size);
    art.rear_wheel.clear(rgba(0, 0, 0, 0));
    art.front_wheel.clear(rgba(0, 0, 0, 0));

    const int rear_cx = static_cast<int>(std::lround(art.rear_x * width));
    const int front_cx = static_cast<int>(std::lround(art.front_x * width));
    const int wheel_cy = static_cast<int>(std::lround(art.wheel_y * height));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Color c = infiltratr_surface_get_pixel(&art.source.get(), x, y);
            if (c.a == 0) continue;

            const int maximum = std::max({static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b)});
            const int minimum = std::min({static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b)});
            const int luma = (static_cast<int>(c.r) + c.g + c.b) / 3;
            const bool paintable = maximum - minimum < 22 &&
                                   luma >= art.paint_min && luma <= art.paint_max;

            const int rear_dx = x - rear_cx;
            const int front_dx = x - front_cx;
            const int wheel_dy = y - wheel_cy;
            const bool rear = rear_dx * rear_dx + wheel_dy * wheel_dy <= source_radius * source_radius;
            const bool front = front_dx * front_dx + wheel_dy * wheel_dy <= source_radius * source_radius;

            // Only non-body pixels may become rotating wheel pixels. This keeps
            // painted fenders and quarter panels rigid even where they overlap
            // the circular wheel extraction area.
            if ((rear || front) && !paintable) {
                SurfaceOwner& wheel = rear ? art.rear_wheel : art.front_wheel;
                const int center_x = rear ? rear_cx : front_cx;
                infiltratr_surface_set_pixel(&wheel.get(),
                    x - center_x + source_radius,
                    y - wheel_cy + source_radius, c);
                continue;
            }

            if (paintable) {
                infiltratr_surface_set_pixel(&art.body.get(), x, y, c);
            } else {
                infiltratr_surface_set_pixel(&art.details.get(), x, y, c);
            }
        }
    }
}

struct VehicleCache {
    std::string id;
    std::uint32_t paint_key = 0;
    int width = 0;
    int height = 0;
    int rear_cx = 0;
    int front_cx = 0;
    int wheel_cy = 0;
    int wheel_radius = 1;
    SurfaceOwner body;
    SurfaceOwner details;
    SurfaceOwner rear_wheel;
    SurfaceOwner front_wheel;
    SurfaceOwner shadow;
};

std::uint32_t paint_key(const PaintState& paint) {
    return (static_cast<std::uint32_t>(paint.r) << 16U) |
           (static_cast<std::uint32_t>(paint.g) << 8U) |
           static_cast<std::uint32_t>(paint.b);
}

std::optional<OwnedCar> copy_active(const GameState& game) {
    if (const OwnedCar* car = game.active_car()) return *car;
    return std::nullopt;
}

} // namespace

struct PixelApp::Impl {
    enum class Screen { Menu, Classifieds, Garage, Parts, Diner, Race, Result, Settings };
    enum class Motion { Idle, Entering, Exiting };

    GameState game;
    DragRaceSession race;
    Opponent opponent;
    RaceResult result;
    SurfaceOwner frame;
    SurfaceOwner garage_source;
    SurfaceOwner garage_scaled;
    GlyphAtlas glyphs;
    std::vector<CarArt> car_art;
    VehicleCache vehicle_cache;

    Screen screen = Screen::Menu;
    Motion motion = Motion::Idle;
    bool running = true;
    bool dirty = true;
    int width = 1280;
    int height = 720;
    int mouse_x = 0;
    int mouse_y = 0;
    std::string message = "BUY A CAR. BUILD IT. RACE IT.";

    std::optional<OwnedCar> displayed;
    std::optional<OwnedCar> incoming;
    std::optional<std::size_t> displayed_index;
    std::optional<std::size_t> incoming_index;
    double car_x = 0.0;
    double wheel_angle = 0.0;

    bool pink_slip = false;
    int wager = 0;
    bool race_settled = false;

    explicit Impl(int w, int h) : frame(w, h), width(w), height(h) {
        load_assets();
        resize(w, h);
        if (const OwnedCar* car = game.active_car()) {
            displayed = *car;
            displayed_index = game.active_car_index();
        }
    }

    void load_assets() {
        garage_source = load_indexed_asset(scene_assets::garage);

        CarArt falcon;
        falcon.id = "falcon64";
        falcon.source = load_indexed_asset(scene_assets::falcon);
        falcon.rear_x = 0.225;
        falcon.front_x = 0.775;
        falcon.wheel_y = 0.80;
        falcon.wheel_r = 0.13;
        falcon.paint_min = 120;
        falcon.paint_max = 248;
        split_car_art(falcon);
        car_art.push_back(std::move(falcon));

        CarArt mustang;
        mustang.id = "mustang65";
        mustang.source = load_indexed_asset(scene_assets::mustang_hd);
        mustang.rear_x = 0.207;
        mustang.front_x = 0.765;
        mustang.wheel_y = 0.75;
        mustang.wheel_r = 0.175;
        mustang.paint_min = 70;
        mustang.paint_max = 205;
        split_car_art(mustang);
        car_art.push_back(std::move(mustang));
    }

    CarArt* art_for(const std::string& id) {
        for (auto& art : car_art) if (art.id == id) return &art;
        return nullptr;
    }

    void resize(int w, int h) {
        width = std::max(640, w);
        height = std::max(480, h);
        frame.resize(width, height);
        if (!garage_source.empty()) garage_scaled = scaled_copy(garage_source, width, height);
        vehicle_cache.id.clear();
        dirty = true;
    }

    void text(int x, int y, const std::string& value,