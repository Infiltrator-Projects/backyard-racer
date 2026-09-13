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
            ::backyard_surface_blit_region_scaled_smooth(
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