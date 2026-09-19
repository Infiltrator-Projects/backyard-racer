            }
        }
        // The old 5x7 bootstrap glyphs keep this split source self-contained,
        // but the shipped UI uses a real antialiased raster atlas.
        base_ = load_indexed_asset(scene_assets::ui_font);
        make_tone(pale_, rgba(239, 231, 207));
        make_tone(gold_, rgba(242, 211, 154));
        make_tone(red_, rgba(236, 106, 86));
        make_tone(dark_, rgba(92, 92, 88));
        make_tone(ink_, rgba(28, 27, 24));
    }

    enum class Tone { Pale, Gold, Red, Dark, Ink };

    void draw(InfiltratrSurface& destination, int x, int y,
              const std::string& value, Tone tone, int scale = 2) const {
        const InfiltratrSurface& atlas = surface_for(tone).get();
        const int target_h = std::max(1, logical_height_ * scale);
        const double factor = static_cast<double>(target_h) / cell_h_;
        const int target_cell_w = std::max(1,
            static_cast<int>(std::lround(cell_w_ * factor)));
        int cursor = x;
        for (char c : value) {
            std::size_t index = characters_.find(c);
            if (index == std::string::npos) index = characters_.find('?');
            ::infiltratr_surface_blit_region_scaled_bilinear(
                &destination, &atlas,
                static_cast<int>(index) * cell_w_, 0, cell_w_, cell_h_,
                cursor, y, target_cell_w, target_h);
            cursor += advance(index, scale);
        }
    }

    int width(const std::string& value, int scale = 2) const {
        int total = 0;
        for (char c : value) {
            std::size_t index = characters_.find(c);
            if (index == std::string::npos) index = characters_.find('?');
            total += advance(index, scale);
        }
        return total;
    }

private:
    const std::string characters_ =
        " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789$-/:.%+?!(),<>";
    static constexpr int cell_w_ = 32;
    static constexpr int cell_h_ = 32;
    static constexpr int logical_height_ = 10;
    inline static constexpr std::array<std::uint8_t, 77> advances_ = {
        10,20,20,20,22,19,17,22,22,9,9,19,17,25,22,23,18,23,20,19,18,21,20,28,20,18,20,
        18,19,16,19,18,11,19,19,9,9,17,9,28,19,18,19,19,13,16,12,19,17,24,17,17,16,
        19,19,19,19,19,19,19,19,19,19,11,11,11,10,27,24,16,12,12,12,10,24,24
    };
    SurfaceOwner base_;
    SurfaceOwner pale_;
    SurfaceOwner gold_;
    SurfaceOwner red_;
    SurfaceOwner dark_;
    SurfaceOwner ink_;

    int advance(std::size_t index, int scale) const {
        const double factor = static_cast<double>(logical_height_ * scale) / cell_h_;
        return std::max(2, static_cast<int>(std::lround(advances_.at(index) * factor)));
    }

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
            case Tone::Ink: return ink_;
            case Tone::Pale: default: return pale_;
        }
    }
};


inline constexpr int kVehicleMasterWidth = 1024;
inline constexpr int kVehicleMasterHeight = 320;
inline constexpr int kVehicleMaxDisplayWidth = 860;

enum class PaintMaskKind { Hue, LightNeutral, DarkNeutral };

struct PhotoCarProfile {
    std::string_view id;
    int sheet_row;
    double front_x;
    double rear_x;
    double wheel_y;
    double wheel_r;
    PaintMaskKind paint_kind;
    double reference_hue;
};

struct CarArt {
    std::string id;
    SurfaceOwner source;
    SurfaceOwner body;
    SurfaceOwner details;
    SurfaceOwner rear_wheel;
    SurfaceOwner front_wheel;
    SurfaceOwner rear_bumper;
    SurfaceOwner front_bumper;
    double rear_x = 0.78;
    double front_x = 0.21;
    double wheel_y = 0.76;
    double wheel_r = 0.145;
};

double channel_value(std::uint8_t value) {
    return static_cast<double>(value) / 255.0;
}

double pixel_value(Color c) {
    return std::max({channel_value(c.r), channel_value(c.g), channel_value(c.b)});
}

double pixel_saturation(Color c) {
    const double r = channel_value(c.r);
    const double g = channel_value(c.g);
    const double b = channel_value(c.b);
    const double hi = std::max({r, g, b});
    const double lo = std::min({r, g, b});
    return hi <= 0.0001 ? 0.0 : (hi - lo) / hi;
}

double pixel_hue(Color c) {
    const double r = channel_value(c.r);
    const double g = channel_value(c.g);
    const double b = channel_value(c.b);
    const double hi = std::max({r, g, b});
    const double lo = std::min({r, g, b});
    const double delta = hi - lo;
    if (delta <= 0.0001) return 0.0;

    double hue = 0.0;
    if (hi == r) hue = 60.0 * std::fmod((g - b) / delta, 6.0);
    else if (hi == g) hue = 60.0 * (((b - r) / delta) + 2.0);
    else hue = 60.0 * (((r - g) / delta) + 4.0);
    if (hue < 0.0) hue += 360.0;
    return hue;
}

double hue_distance(double a, double b) {
    const double raw = std::abs(a - b);
    return std::min(raw, 360.0 - raw);
}

Color surface_pixel(const SurfaceOwner& surface, int x, int y) {
    if (surface.empty() || x < 0 || y < 0 ||
        x >= surface.width() || y >= surface.height()) {
        return rgba(0, 0, 0, 0);
    }
    return surface.get().pixels[
        static_cast<std::size_t>(y) * surface.get().width +
        static_cast<std::size_t>(x)];
}

SurfaceOwner crop_surface(const SurfaceOwner& source, int x0, int y0,
                          int width, int height) {
    SurfaceOwner result(width, height);
    result.clear(rgba(0, 0, 0, 0));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Color c = surface_pixel(source, x0 + x, y0 + y);
            if (c.a == 0) continue;
            infiltratr_surface_set_pixel(&result.get(), x, y, c);
        }
    }
    return result;
}

bool in_wheel_circle(const PhotoCarProfile& p, int x, int y,
                     int width, int height, bool front, double radius_scale = 1.0) {
    const double cx = (front ? p.front_x : p.rear_x) * width;
    const double cy = p.wheel_y * height;
    const double radius = p.wheel_r * height * radius_scale;
    const double dx = x - cx;
    const double dy = y - cy;
    return dx * dx + dy * dy <= radius * radius;
}

bool in_glass_zone(int x, int y, int width, int height) {
    const double nx = static_cast<double>(x) / std::max(1, width - 1);
    const double ny = static_cast<double>(y) / std::max(1, height - 1);
    return nx > 0.255 && nx < 0.755 && ny > 0.12 && ny < 0.47;
}

bool bumper_pixel(Color c, int x, int y, int width, int height, bool front) {
    const double nx = static_cast<double>(x) / std::max(1, width - 1);
    const double ny = static_cast<double>(y) / std::max(1, height - 1);
    const bool end_zone = front ? (nx <= 0.125) : (nx >= 0.875);
    if (!end_zone || ny < 0.57 || ny > 0.84 || c.a == 0) return false;
    return pixel_saturation(c) < 0.24 && pixel_value(c) > 0.42;
}

bool body_paint_pixel(const PhotoCarProfile& p, Color c, int x, int y,
                      int width, int height) {
    if (c.a == 0) return false;
    if (in_wheel_circle(p, x, y, width, height, true, 1.02) ||
        in_wheel_circle(p, x, y, width, height, false, 1.02)) {
        return false;
    }
    if (bumper_pixel(c, x, y, width, height, true) ||
        bumper_pixel(c, x, y, width, height, false)) {
        return false;
    }

    const double nx = static_cast<double>(x) / std::max(1, width - 1);
    const double ny = static_cast<double>(y) / std::max(1, height - 1);
    if (nx < 0.015 || nx > 0.985 || ny < 0.08 || ny > 0.84) return false;

    const double value = pixel_value(c);
    const double saturation = pixel_saturation(c);

    switch (p.paint_kind) {
        case PaintMaskKind::Hue:
            return saturation >= 0.16 && value >= 0.10 &&
                   hue_distance(pixel_hue(c), p.reference_hue) <= 46.0;
        case PaintMaskKind::LightNeutral:
            if (in_glass_zone(x, y, width, height)) return false;
            return saturation <= 0.24 && value >= 0.30 && value <= 0.91;
        case PaintMaskKind::DarkNeutral:
            if (in_glass_zone(x, y, width, height)) return false;
            return saturation <= 0.25 && value >= 0.045 && value <= 0.52;
    }
    return false;
}

SurfaceOwner extract_wheel(const SurfaceOwner& source,
                           const PhotoCarProfile& p, bool front) {
    const int cx = static_cast<int>(std::lround(
        (front ? p.front_x : p.rear_x) * source.width()));
    const int cy = static_cast<int>(std::lround(p.wheel_y * source.height()));
    const int radius = std::max(8, static_cast<int>(
        std::lround(p.wheel_r * source.height())));
    const int size = radius * 2 + 4;
    SurfaceOwner wheel(size, size);
    wheel.clear(rgba(0, 0, 0, 0));
    for (int sy = cy - radius; sy <= cy + radius; ++sy) {
        for (int sx = cx - radius; sx <= cx + radius; ++sx) {
            const double dx = sx - cx;
            const double dy = sy - cy;
            if (dx * dx + dy * dy > radius * radius * 0.98) continue;
            const Color c = surface_pixel(source, sx, sy);
            if (c.a == 0) continue;
            infiltratr_surface_set_pixel(&wheel.get(),
                sx - (cx - radius) + 2,
                sy - (cy - radius) + 2, c);
        }
    }
    return wheel;
}

CarArt make_photo_car_art(const SurfaceOwner& sheet, const PhotoCarProfile& p) {
    constexpr int sheet_width = 512;
    constexpr int row_height = 160;

    SurfaceOwner row = crop_surface(
        sheet, 0, p.sheet_row * row_height, sheet_width, row_height);
    SurfaceOwner source = scaled_copy(row, kVehicleMasterWidth, kVehicleMasterHeight);

    CarArt art;
    art.id = std::string(p.id);
    art.front_x = p.front_x;
    art.rear_x = p.rear_x;
    art.wheel_y = p.wheel_y;
    art.wheel_r = p.wheel_r;
    art.source = std::move(source);

    art.body.resize(art.source.width(), art.source.height());
    art.details.resize(art.source.width(), art.source.height());
    art.front_bumper.resize(art.source.width(), art.source.height());
    art.rear_bumper.resize(art.source.width(), art.source.height());
    art.body.clear(rgba(0, 0, 0, 0));
    art.details.clear(rgba(0, 0, 0, 0));
    art.front_bumper.clear(rgba(0, 0, 0, 0));
    art.rear_bumper.clear(rgba(0, 0, 0, 0));

    for (int y = 0; y < art.source.height(); ++y) {
        for (int x = 0; x < art.source.width(); ++x) {
            const Color c = surface_pixel(art.source, x, y);
            if (c.a == 0) continue;

            const bool front_wheel =
                in_wheel_circle(p, x, y, art.source.width(), art.source.height(),
                                true, 0.99);
            const bool rear_wheel =
                in_wheel_circle(p, x, y, art.source.width(), art.source.height(),
                                false, 0.99);
            if (front_wheel || rear_wheel) continue;

            const bool front_bumper =
                bumper_pixel(c, x, y, art.source.width(), art.source.height(), true);
            const bool rear_bumper =
                bumper_pixel(c, x, y, art.source.width(), art.source.height(), false);
            if (front_bumper) {
                infiltratr_surface_set_pixel(&art.front_bumper.get(), x, y, c);
                continue;
            }
            if (rear_bumper) {
                infiltratr_surface_set_pixel(&art.rear_bumper.get(), x, y, c);
                continue;
            }

            if (body_paint_pixel(p, c, x, y,
                                 art.source.width(), art.source.height())) {
                const std::uint8_t luma = static_cast<std::uint8_t>(std::clamp(
                    static_cast<int>(std::lround(
                        0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b)), 0, 255));
                infiltratr_surface_set_pixel(&art.body.get(), x, y,
                                             rgba(luma, luma, luma, c.a));
            } else {
                infiltratr_surface_set_pixel(&art.details.get(), x, y, c);
            }
        }
    }

    art.front_wheel = extract_wheel(art.source, p, true);
    art.rear_wheel = extract_wheel(art.source, p, false);
    return art;
}

static constexpr std::array<PhotoCarProfile, 12> kPhotoCarProfiles{{
    // id              row front rear  wheelY radius mask                           reference hue
    {"falcon64",         0, 0.205,0.785,0.765,0.145, PaintMaskKind::LightNeutral,   0.0},
    {"mustang65",        1, 0.195,0.790,0.770,0.145, PaintMaskKind::Hue,             2.0},
    {"nova66",           2, 0.195,0.790,0.770,0.145, PaintMaskKind::Hue,           208.0},
    {"camaro67",         3, 0.195,0.790,0.770,0.150, PaintMaskKind::Hue,           138.0},
    {"charger68",        4, 0.195,0.795,0.770,0.150, PaintMaskKind::DarkNeutral,     0.0},
    {"cougar68",         5, 0.190,0.790,0.770,0.150, PaintMaskKind::Hue,            40.0},
    {"gto69",            6, 0.195,0.790,0.770,0.150, PaintMaskKind::Hue,             4.0},
    {"roadrunner69",     7, 0.195,0.795,0.770,0.150, PaintMaskKind::Hue,           214.0},
    {"torino69",         8, 0.195,0.795,0.770,0.150, PaintMaskKind::Hue,           278.0},
    {"challenger70",     9, 0.190,0.800,0.775,0.150, PaintMaskKind::Hue,            22.0},
    {"chevelle70",      10, 0.200,0.795,0.775,0.150, PaintMaskKind::LightNeutral,    0.0},
    {"barracuda70",     11, 0.195,0.800,0.775,0.150, PaintMaskKind::Hue,            92.0}
}};

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
    SurfaceOwner rear_bumper;
    SurfaceOwner front_bumper;
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
    enum class Screen {
        Menu, Classifieds, ClassifiedArticle, Garage, Parts, Diner, Race, Result, Settings
    };
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
    std::optional<std::size_t> selected_classified_index;

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
        garage_source = load_indexed_asset(scene_assets::garage_hd);
        SurfaceOwner vehicle_sheet =
            load_indexed_asset(scene_assets::vehicle_photo_sheet_hd);

        car_art.clear();
        car_art.reserve(kPhotoCarProfiles.size());
        for (const PhotoCarProfile& profile : kPhotoCarProfiles) {
            car_art.push_back(make_photo_car_art(vehicle_sheet, profile));
        }
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