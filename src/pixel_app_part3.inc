              GlyphAtlas::Tone tone = GlyphAtlas::Tone::Pale, int scale = 2) {
        glyphs.draw(frame.get(), x, y, value, tone, scale);
    }

    void text_center(Rect area, const std::string& value,
                     GlyphAtlas::Tone tone = GlyphAtlas::Tone::Pale, int scale = 2) {
        const int tw = glyphs.width(value, scale);
        text(area.x + (area.w - tw) / 2,
             area.y + (area.h - 10 * scale) / 2,
             value, tone, scale);
    }

    void panel(Rect r) {
        frame.blend(r.x, r.y, r.w, r.h, rgba(10, 10, 10, 222));
        outline(frame.get(), r.x, r.y, r.w, r.h, rgba(218, 203, 170), 2);
    }

    void button(Rect r, const std::string& label, bool enabled = true) {
        const bool hover = enabled && r.contains(mouse_x, mouse_y);
        frame.blend(r.x, r.y, r.w, r.h,
                    hover ? rgba(118, 36, 31, 238) : rgba(24, 22, 21, 232));
        outline(frame.get(), r.x, r.y, r.w, r.h,
                enabled ? rgba(225, 209, 170) : rgba(92, 92, 88), 2);
        text_center(r, label,
                    enabled ? GlyphAtlas::Tone::Pale : GlyphAtlas::Tone::Dark, 2);
    }

    void status(const std::string& where) {
        frame.blend(0, 0, width, 66, rgba(9, 9, 10, 238));
        frame.fill(0, 62, width, 4, rgba(157, 37, 29));
        text(22, 12, "BACKYARD RACER", GlyphAtlas::Tone::Gold, 3);
        text(22, 42, where, GlyphAtlas::Tone::Pale, 2);
        const std::string right = "CASH $" + std::to_string(game.cash()) +
            "  REP " + std::to_string(game.reputation()) +
            "  W " + std::to_string(game.wins()) +
            " L " + std::to_string(game.losses());
        text(std::max(520, width - glyphs.width(right, 2) - 24), 22, right,
             GlyphAtlas::Tone::Pale, 2);
    }

    int model_width(const OwnedCar& car) const {
        if (car.base.id == "falcon64" || car.base.id == "nova66")
            return std::clamp(width * 59 / 100, 560, 790);
        if (car.base.id == "charger68" || car.base.id == "torino69")
            return std::clamp(width * 64 / 100, 600, 860);
        if (car.base.id == "gto69" || car.base.id == "roadrunner69" ||
            car.base.id == "chevelle70")
            return std::clamp(width * 62 / 100, 580, 830);
        return std::clamp(width * 60 / 100, 570, 810);
    }

    void build_vehicle_cache(const OwnedCar& car, int draw_width) {
        CarArt* art = art_for(car.base.id);
        if (!art) return;
        const std::uint32_t key = paint_key(car.paint);
        if (vehicle_cache.id == car.base.id && vehicle_cache.paint_key == key &&
            vehicle_cache.width == draw_width) return;

        vehicle_cache = VehicleCache{};
        vehicle_cache.id = car.base.id;
        vehicle_cache.paint_key = key;
        vehicle_cache.width = draw_width;
        vehicle_cache.height = std::max(1, art->source.height() * draw_width / art->source.width());

        SurfaceOwner tinted;
        if (!infiltratr_surface_copy_luma_tinted(&tinted.get(), &art->body.get(),
                rgba(car.paint.r, car.paint.g, car.paint.b))) {
            throw std::bad_alloc();
        }
        vehicle_cache.body = scaled_copy(tinted, draw_width, vehicle_cache.height);
        vehicle_cache.details = scaled_copy(art->details, draw_width, vehicle_cache.height);

        const double scale = static_cast<double>(draw_width) / art->source.width();
        const int rear_w = std::max(1, static_cast<int>(std::lround(art->rear_wheel.width() * scale)));
        const int rear_h = std::max(1, static_cast<int>(std::lround(art->rear_wheel.height() * scale)));
        const int front_w = std::max(1, static_cast<int>(std::lround(art->front_wheel.width() * scale)));
        const int front_h = std::max(1, static_cast<int>(std::lround(art->front_wheel.height() * scale)));
        vehicle_cache.rear_wheel = scaled_copy(art->rear_wheel, rear_w, rear_h);
        vehicle_cache.front_wheel = scaled_copy(art->front_wheel, front_w, front_h);
        vehicle_cache.rear_bumper =
            scaled_copy(art->rear_bumper, draw_width, vehicle_cache.height);
        vehicle_cache.front_bumper =
            scaled_copy(art->front_bumper, draw_width, vehicle_cache.height);

        vehicle_cache.rear_cx = static_cast<int>(std::lround(art->rear_x * draw_width));
        vehicle_cache.front_cx = static_cast<int>(std::lround(art->front_x * draw_width));
        vehicle_cache.wheel_cy = static_cast<int>(std::lround(art->wheel_y * vehicle_cache.height));
        vehicle_cache.wheel_radius = std::max(1, rear_w / 2);

        const int shadow_h = std::max(10, vehicle_cache.height / 13);
        vehicle_cache.shadow.resize(draw_width, shadow_h);
        vehicle_cache.shadow.clear(rgba(0, 0, 0, 0));
        for (int y = 0; y < shadow_h; ++y) {
            const double normalized = std::abs((2.0 * y / std::max(1, shadow_h - 1)) - 1.0);
            const double profile = std::sqrt(std::max(0.0, 1.0 - normalized * normalized));
            const int row_w = std::max(1, static_cast<int>(draw_width * 0.72 * profile));
            const int x = (draw_width - row_w) / 2;
            vehicle_cache.shadow.fill(x, y, row_w, 1,
                                      rgba(0, 0, 0, static_cast<std::uint8_t>(70 * profile)));
        }
    }

    void draw_car(const OwnedCar& car, int x, int base_y, double angle) {
        CarArt* art = art_for(car.base.id);
        if (!art) {
            panel({width / 2 - 220, base_y - 130, 440, 90});
            text(width / 2 - 190, base_y - 98, "MODEL ART PENDING",
                 GlyphAtlas::Tone::Gold, 2);
            return;
        }
        const int draw_width = model_width(car);
        build_vehicle_cache(car, draw_width);
        const int y = base_y - vehicle_cache.height;

        frame.blit(vehicle_cache.shadow, x, base_y - vehicle_cache.shadow.height() / 2);
        frame.blit(vehicle_cache.body, x, y);
        infiltratr_surface_blit_rotated(&frame.get(), &vehicle_cache.rear_wheel.get(),
            x + vehicle_cache.rear_cx, y + vehicle_cache.wheel_cy, angle);
        infiltratr_surface_blit_rotated(&frame.get(), &vehicle_cache.front_wheel.get(),
            x + vehicle_cache.front_cx, y + vehicle_cache.wheel_cy, angle);
        frame.blit(vehicle_cache.details, x, y);
        if (car.rear_bumper_installed) frame.blit(vehicle_cache.rear_bumper, x, y);
        if (car.front_bumper_installed) frame.blit(vehicle_cache.front_bumper, x, y);
    }

    double parked_x(const OwnedCar& car) const {
        return static_cast<double>((width - model_width(car)) / 2 + 35);
    }

    void begin_change(const std::optional<OwnedCar>& old_car,
                      std::optional<std::size_t> old_index,
                      const std::optional<OwnedCar>& new_car,
                      std::optional<std::size_t> new_index) {
        incoming = new_car;
        incoming_index = new_index;
        if (old_car) {
            displayed = old_car;
            displayed_index = old_index;
            car_x = parked_x(*old_car);
            motion = Motion::Exiting;
        } else if (new_car) {
            displayed = new_car;
            displayed_index = new_index;
            car_x = static_cast<double>(width + 40);
            motion = Motion::Entering;
        } else {
            displayed.reset();
            displayed_index.reset();
            incoming.reset();
            incoming_index.reset();
            motion = Motion::Idle;
        }
        dirty = true;
    }

    void sync_garage_to_active() {
        if (motion != Motion::Idle) return;
        const auto active = copy_active(game);
        const std::optional<std::size_t> active_index = active ?
            std::optional<std::size_t>(game.active_car_index()) : std::nullopt;
        if (displayed_index == active_index) {
            if (active) displayed = active;
            return;
        }
        begin_change(displayed, displayed_index, active, active_index);
    }

    bool tick_motion(double dt) {
        if (motion == Motion::Idle) return false;
        if (!displayed) {
            motion = Motion::Idle;
            return true;
        }
        dt = std::clamp(dt, 0.0, 0.05);
        const int car_width = model_width(*displayed);
        const double radius = std::max(20.0, car_width * 0.105);

        if (motion == Motion::Exiting) {
            const double dx = std::min(760.0 * dt,
                                       static_cast<double>(width + car_width) - car_x);
            car_x += dx;
            wheel_angle -= dx / radius;
            if (car_x >= width + 10) {
                if (incoming) {
                    displayed = incoming;
                    displayed_index = incoming_index;
                    incoming.reset();
                    incoming_index.reset();
                    car_x = static_cast<double>(width + 30);
                    motion = Motion::Entering;
                } else {
                    displayed.reset();
                    displayed_index.reset();
                    motion = Motion::Idle;
                }
            }
        } else {
            const double target = parked_x(*displayed);
            const double remaining = car_x - target;
            double speed = 720.0;
            if (remaining < 260.0) speed = std::max(170.0, remaining * 2.4);
            const double dx = std::min(remaining, speed * dt);
            car_x -= dx;
            wheel_angle += dx / radius;
            if (car_x <= target + 0.5) {
                car_x = target;
                motion = Motion::Idle;
                if (const auto active = copy_active(game)) {
                    displayed = active;
                    displayed_index = game.active_car_index();
                }
            }
        }
        return true;
    }

    Rect menu_new() const { return {72, 252, 300, 48}; }
    Rect menu_continue() const { return {72, 312, 300, 48}; }
    Rect menu_settings() const { return {72, 372, 300, 48}; }
    Rect menu_quit() const { return {72, 432, 300, 48}; }

    void menu_button(Rect r, const std::string& label, bool enabled = true) {
        const bool hover = enabled && r.contains(mouse_x, mouse_y);
        if (enabled) {
            frame.blend(r.x, r.y, r.w, r.h,
                        hover ? rgba(17, 17, 18, 226) : rgba(8, 8, 9, 174));
            frame.fill(r.x, r.y, hover ? 6 : 3, r.h,
                       hover ? rgba(166, 45, 35) : rgba(177, 146, 92));
            frame.fill(r.x + 14, r.y + r.h - 1, r.w - 14, 1, rgba(83, 79, 71));
            text(r.x + 22, r.y + 13, label,
                 hover ? GlyphAtlas::Tone::Gold : GlyphAtlas::Tone::Pale, 2);
        } else {
            frame.blend(r.x, r.y, r.w, r.h, rgba(8, 8, 9, 112));
            frame.fill(r.x, r.y, 3, r.h, rgba(70, 67, 62));
            text(r.x + 22, r.y + 13, label, GlyphAtlas::Tone::Dark, 2);
        }
    }

    void draw_menu() {
        if (!garage_scaled.empty()) infiltratr_surface_copy(&frame.get(), &garage_scaled.get());
        else frame.clear(rgba(37, 34, 31));

        // Preserve the garage as the hero image. The menu now sits in a graded
        // left-side scrim instead of a giant opaque box covering the artwork.
        frame.blend(0, 0, width, height, rgba(0, 0, 0, 18));
        for (int i = 0; i < 7; ++i) {
            const int alpha = std::max(0, 162 - i * 22);
            frame.blend(i * 72, 0, 72, height,
                        rgba(0, 0, 0, static_cast<std::uint8_t>(alpha)));
        }

        text(74, 103, "BACKYARD RACER", GlyphAtlas::Tone::Dark, 3);
        text(72, 101, "BACKYARD RACER", GlyphAtlas::Tone::Gold, 3);
        text(72, 151, "Build it. Race it. Risk it.", GlyphAtlas::Tone::Pale, 2);
        frame.fill(72, 190, 72, 3, rgba(166, 45, 35));

        menu_button(menu_new(), "New Game");
        menu_button(menu_continue(), "Continue", game.started());
        menu_button(menu_settings(), "Settings");
        menu_button(menu_quit(), "Quit");

        const std::string footer = game.started() ?
            "Continue where you left off." : "Buy a car. Build it. Race it.";
        text(72, 524, footer, GlyphAtlas::Tone::Pale, 2);
        if (!message.empty()) text(72, 558, message, GlyphAtlas::Tone::Dark, 1);
    }

    void draw_classifieds() {
        frame.clear(rgba(207, 198, 168));
        frame.fill(20, 20, width - 40, height - 40, rgba(236, 229, 202));
        outline(frame.get(), 20, 20, width - 40, height - 40, rgba(44, 40, 34), 3);
        text(48, 42, "THE BACKYARD GAZETTE", GlyphAtlas::Tone::Dark, 3);
        frame.fill(48, 72, width - 96, 3, rgba(44, 40, 34));
        text(48, 88, "USED CARS - CASH TALKS - CLICK AN AD TO BUY", GlyphAtlas::Tone::Dark, 2);

        const auto& cars = game.classifieds();
        const int gap = 14;
        const int left = 48;
        const int top = 112;
        const int card_w = (width - 110) / 2;
        const int card_h = 112;
        for (std::size_t i = 0; i < cars.size(); ++i) {
            const int col = static_cast<int>(i % 2);
