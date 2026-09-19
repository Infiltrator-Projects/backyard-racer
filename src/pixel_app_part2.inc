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


struct CarProfile {
    std::string_view id;
    double front_x;
    double rear_x;
    double wheel_y;
    double wheel_r;
    double hood_y;
    double trunk_y;
    double roof_y;
    double windshield_base_x;
    double roof_front_x;
    double roof_rear_x;
    double backlight_base_x;
    double rocker_y;
    double door_x;
    bool fastback;
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
    double wheel_y = 0.77;
    double wheel_r = 0.145;
};

struct HdPoint {
    double x = 0.0;
    double y = 0.0;
};

Color mix_color(Color a, Color b, double t) {
    t = std::clamp(t, 0.0, 1.0);
    const auto ch = [t](std::uint8_t x, std::uint8_t y) {
        return static_cast<std::uint8_t>(std::lround(
            static_cast<double>(x) + (static_cast<double>(y) - x) * t));
    };
    return rgba(ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b), ch(a.a, b.a));
}

void fill_polygon_gradient(SurfaceOwner& surface,
                           const std::vector<HdPoint>& points,
                           Color top, Color bottom) {
    if (points.size() < 3 || surface.empty()) return;
    double min_y = points.front().y;
    double max_y = points.front().y;
    for (const auto& p : points) {
        min_y = std::min(min_y, p.y);
        max_y = std::max(max_y, p.y);
    }
    const int y0 = std::max(0, static_cast<int>(std::floor(min_y)));
    const int y1 = std::min(surface.height() - 1, static_cast<int>(std::ceil(max_y)));
    const double denom = std::max(1.0, max_y - min_y);

    std::vector<double> xs;
    xs.reserve(points.size());
    for (int y = y0; y <= y1; ++y) {
        const double scan_y = y + 0.5;
        xs.clear();
        for (std::size_t i = 0; i < points.size(); ++i) {
            const HdPoint a = points[i];
            const HdPoint b = points[(i + 1) % points.size()];
            const bool crosses =
                (a.y <= scan_y && b.y > scan_y) ||
                (b.y <= scan_y && a.y > scan_y);
            if (!crosses) continue;
            const double t = (scan_y - a.y) / (b.y - a.y);
            xs.push_back(a.x + (b.x - a.x) * t);
        }
        std::sort(xs.begin(), xs.end());
        const Color row = mix_color(top, bottom, (scan_y - min_y) / denom);
        for (std::size_t i = 0; i + 1 < xs.size(); i += 2) {
            int x0 = static_cast<int>(std::ceil(xs[i]));
            int x1 = static_cast<int>(std::floor(xs[i + 1]));
            x0 = std::clamp(x0, 0, surface.width() - 1);
            x1 = std::clamp(x1, 0, surface.width() - 1);
            if (x1 >= x0) surface.fill(x0, y, x1 - x0 + 1, 1, row);
        }
    }
}

void fill_polygon(SurfaceOwner& surface,
                  const std::vector<HdPoint>& points,
                  Color color) {
    fill_polygon_gradient(surface, points, color, color);
}

void fill_ellipse(SurfaceOwner& surface, int cx, int cy, int rx, int ry, Color color) {
    if (rx <= 0 || ry <= 0 || surface.empty()) return;
    const int y0 = std::max(0, cy - ry);
    const int y1 = std::min(surface.height() - 1, cy + ry);
    for (int y = y0; y <= y1; ++y) {
        const double dy = static_cast<double>(y - cy) / ry;
        const double inside = std::max(0.0, 1.0 - dy * dy);
        const int half = static_cast<int>(std::lround(rx * std::sqrt(inside)));
        const int x0 = std::max(0, cx - half);
        const int x1 = std::min(surface.width() - 1, cx + half);
        if (x1 >= x0) surface.fill(x0, y, x1 - x0 + 1, 1, color);
    }
}

void draw_disc(SurfaceOwner& surface, int cx, int cy, int r, Color color) {
    fill_ellipse(surface, cx, cy, r, r, color);
}

void draw_line_hd(SurfaceOwner& surface, int x0, int y0, int x1, int y1,
                  int width, Color color) {
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps <= 0) {
        draw_disc(surface, x0, y0, std::max(1, width / 2), color);
        return;
    }
    const int radius = std::max(1, width / 2);
    for (int i = 0; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps;
        const int x = static_cast<int>(std::lround(x0 + dx * t));
        const int y = static_cast<int>(std::lround(y0 + dy * t));
        draw_disc(surface, x, y, radius, color);
    }
}

std::vector<HdPoint> car_body_outline(const CarProfile& p, int w, int h) {
    const auto X = [w](double v) { return v * w; };
    const auto Y = [h](double v) { return v * h; };
    return {
        {X(0.018), Y(p.hood_y + 0.025)},
        {X(0.045), Y(p.hood_y - 0.010)},
        {X(p.windshield_base_x - 0.018), Y(p.hood_y - 0.018)},
        {X(p.roof_front_x), Y(p.roof_y + 0.012)},
        {X(p.roof_rear_x), Y(p.roof_y)},
        {X(p.backlight_base_x), Y(p.fastback ? p.trunk_y - 0.045 : p.trunk_y - 0.020)},
        {X(0.940), Y(p.trunk_y)},
        {X(0.982), Y(p.trunk_y + 0.035)},
        {X(0.986), Y(p.rocker_y - 0.085)},
        {X(0.955), Y(p.rocker_y)},
        {X(0.045), Y(p.rocker_y)},
        {X(0.015), Y(p.rocker_y - 0.095)}
    };
}

std::vector<HdPoint> car_window_outline(const CarProfile& p, int w, int h) {
    const auto X = [w](double v) { return v * w; };
    const auto Y = [h](double v) { return v * h; };
    return {
        {X(p.windshield_base_x + 0.012), Y(p.hood_y - 0.028)},
        {X(p.roof_front_x + 0.012), Y(p.roof_y + 0.035)},
        {X(p.roof_rear_x - 0.012), Y(p.roof_y + 0.025)},
        {X(p.backlight_base_x - 0.014), Y((p.fastback ? p.trunk_y - 0.060 : p.trunk_y - 0.035))}
    };
}

SurfaceOwner make_hd_wheel(int final_size) {
    constexpr int ss = 2;
    const int size = std::max(24, final_size);
    SurfaceOwner hi(size * ss, size * ss);
    hi.clear(rgba(0, 0, 0, 0));
    const int c = hi.width() / 2;
    const int r = c - 2 * ss;

    draw_disc(hi, c, c, r, rgba(18, 19, 21));
    draw_disc(hi, c, c, static_cast<int>(r * 0.78), rgba(58, 61, 65));
    draw_disc(hi, c, c, static_cast<int>(r * 0.62), rgba(202, 207, 212));
    draw_disc(hi, c, c, static_cast<int>(r * 0.50), rgba(31, 33, 36));

    for (int spoke = 0; spoke < 5; ++spoke) {
        const double a = -1.57079632679 + spoke * 1.25663706144;
        const int x0 = c + static_cast<int>(std::cos(a) * r * 0.12);
        const int y0 = c + static_cast<int>(std::sin(a) * r * 0.12);
        const int x1 = c + static_cast<int>(std::cos(a) * r * 0.48);
        const int y1 = c + static_cast<int>(std::sin(a) * r * 0.48);
        draw_line_hd(hi, x0, y0, x1, y1, 6 * ss, rgba(205, 211, 216));
        draw_line_hd(hi, x0 + ss, y0 + ss, x1 + ss, y1 + ss, 2 * ss, rgba(85, 90, 96));
    }
    draw_disc(hi, c, c, static_cast<int>(r * 0.15), rgba(215, 220, 224));
    draw_disc(hi, c, c, static_cast<int>(r * 0.060), rgba(55, 58, 62));

    return scaled_copy(hi, size, size);
}

void draw_model_details(const CarProfile& p, SurfaceOwner& detail, int w, int h) {
    const auto X = [w](double v) { return static_cast<int>(std::lround(v * w)); };
    const auto Y = [h](double v) { return static_cast<int>(std::lround(v * h)); };

    const auto glass = car_window_outline(p, w, h);
    fill_polygon(detail, glass, rgba(24, 31, 37));
    for (std::size_t i = 0; i < glass.size(); ++i) {
        const auto& a = glass[i];
        const auto& b = glass[(i + 1) % glass.size()];
        draw_line_hd(detail, static_cast<int>(a.x), static_cast<int>(a.y),
                     static_cast<int>(b.x), static_cast<int>(b.y),
                     std::max(2, h / 130), rgba(205, 211, 215));
    }

    const int pillar_x = X(p.roof_front_x + (p.roof_rear_x - p.roof_front_x) * 0.58);
    draw_line_hd(detail, pillar_x, Y(p.roof_y + 0.030),
                 pillar_x, Y(p.hood_y - 0.030),
                 std::max(3, h / 95), rgba(175, 181, 186));

    // Door shut line and handle.
    draw_line_hd(detail, X(p.door_x), Y(p.hood_y - 0.010),
                 X(p.door_x), Y(p.rocker_y - 0.045),
                 std::max(2, h / 150), rgba(65, 68, 72));
    detail.fill(X(p.door_x - 0.035), Y(p.hood_y + 0.070),
                std::max(8, w / 55), std::max(2, h / 100),
                rgba(210, 214, 216));

    // Character line and sill chrome.
    draw_line_hd(detail, X(0.055), Y(p.hood_y + 0.050),
                 X(0.948), Y(p.trunk_y + 0.055),
                 std::max(2, h / 150), rgba(112, 116, 120));
    draw_line_hd(detail, X(0.075), Y(p.rocker_y - 0.020),
                 X(0.925), Y(p.rocker_y - 0.020),
                 std::max(2, h / 130), rgba(190, 195, 198));

    // Front grille/headlight group is always at the left.
    detail.fill(X(0.015), Y(p.hood_y + 0.055), std::max(8, w / 70),
                std::max(16, h / 13), rgba(34, 37, 40));
    draw_disc(detail, X(0.040), Y(p.hood_y + 0.075), std::max(4, h / 42), rgba(226, 220, 170));

    // Rear lamp at the right.
    detail.fill(X(0.955), Y(p.trunk_y + 0.070), std::max(8, w / 75),
                std::max(6, h / 36), rgba(162, 31, 27));

    // Model-specific identity cues.
    const int cue = std::max(2, h / 120);
    if (p.id == "falcon64") {
        for (int i = 0; i < 4; ++i)
            draw_line_hd(detail, X(0.070 + i * 0.009), Y(p.hood_y + 0.030),
                         X(0.070 + i * 0.009), Y(p.hood_y + 0.105), cue, rgba(180, 185, 188));
    } else if (p.id == "mustang65") {
        for (int i = 0; i < 3; ++i)
            draw_line_hd(detail, X(0.690 + i * 0.010), Y(p.hood_y + 0.085),
                         X(0.680 + i * 0.010), Y(p.hood_y + 0.160), cue, rgba(190, 194, 198));
    } else if (p.id == "nova66") {
        draw_line_hd(detail, X(0.060), Y(p.hood_y + 0.025), X(0.935), Y(p.hood_y + 0.025),
                     cue, rgba(205, 208, 210));
    } else if (p.id == "camaro67") {
        for (int i = 0; i < 3; ++i)
            draw_line_hd(detail, X(0.655 + i * 0.012), Y(p.hood_y + 0.090),
                         X(0.648 + i * 0.012), Y(p.hood_y + 0.145), cue, rgba(70, 72, 75));
    } else if (p.id == "charger68") {
        detail.fill(X(0.018), Y(p.hood_y + 0.040), X(0.085), std::max(16, h / 14), rgba(22, 24, 26));
        draw_line_hd(detail, X(0.880), Y(p.trunk_y + 0.020), X(0.880), Y(p.rocker_y - 0.090),
                     cue * 2, rgba(120, 124, 128));
    } else if (p.id == "gto69") {
        detail.fill(X(0.145), Y(p.hood_y - 0.055), X(0.065), std::max(5, h / 55), rgba(54, 56, 58));
    } else if (p.id == "roadrunner69") {
        detail.fill(X(0.125), Y(p.hood_y - 0.070), X(0.090), std::max(6, h / 50), rgba(41, 43, 45));
    } else if (p.id == "challenger70" || p.id == "barracuda70") {
        detail.fill(X(0.140), Y(p.hood_y - 0.075), X(0.085), std::max(7, h / 45), rgba(45, 47, 49));
        draw_line_hd(detail, X(0.835), Y(p.trunk_y + 0.040), X(0.835), Y(p.rocker_y - 0.070),
                     cue * 2, rgba(50, 52, 54));
    } else if (p.id == "cougar68") {
        detail.fill(X(0.018), Y(p.hood_y + 0.035), X(0.075), std::max(18, h / 13), rgba(31, 33, 35));
        for (int i = 0; i < 3; ++i)
            draw_line_hd(detail, X(0.955 + i * 0.007), Y(p.trunk_y + 0.035),
                         X(0.955 + i * 0.007), Y(p.trunk_y + 0.115), cue, rgba(150, 25, 24));
    } else if (p.id == "torino69") {
        draw_line_hd(detail, X(0.085), Y(p.hood_y + 0.020), X(0.910), Y(p.trunk_y + 0.020),
                     cue * 2, rgba(55, 57, 60));
    } else if (p.id == "chevelle70") {
        detail.fill(X(0.118), Y(p.hood_y - 0.055), X(0.065), std::max(5, h / 55), rgba(55, 57, 59));
        detail.fill(X(0.655), Y(p.hood_y + 0.105), std::max(18, w / 38), std::max(8, h / 28),
                    rgba(46, 48, 50));
    }
}

CarArt make_hd_car_art(const CarProfile& p) {
    constexpr int final_w = 960;
    constexpr int final_h = 300;
    constexpr int ss = 2;
    const int w = final_w * ss;
    const int h = final_h * ss;

    SurfaceOwner body_hi(w, h);
    SurfaceOwner detail_hi(w, h);
    SurfaceOwner front_bumper_hi(w, h);
    SurfaceOwner rear_bumper_hi(w, h);
    body_hi.clear(rgba(0, 0, 0, 0));
    detail_hi.clear(rgba(0, 0, 0, 0));
    front_bumper_hi.clear(rgba(0, 0, 0, 0));
    rear_bumper_hi.clear(rgba(0, 0, 0, 0));

    const auto body_outline = car_body_outline(p, w, h);
    fill_polygon_gradient(body_hi, body_outline,
                          rgba(218, 219, 220), rgba(104, 108, 113));

    const int front_cx = static_cast<int>(std::lround(p.front_x * w));
    const int rear_cx = static_cast<int>(std::lround(p.rear_x * w));
    const int wheel_cy = static_cast<int>(std::lround(p.wheel_y * h));
    const int wheel_r = static_cast<int>(std::lround(p.wheel_r * h));

    // True wheel-arch cutouts, not rotating painted fenders.
    fill_ellipse(body_hi, front_cx, wheel_cy, static_cast<int>(wheel_r * 1.07),
                 static_cast<int>(wheel_r * 1.07), rgba(0, 0, 0, 0));
    fill_ellipse(body_hi, rear_cx, wheel_cy, static_cast<int>(wheel_r * 1.07),
                 static_cast<int>(wheel_r * 1.07), rgba(0, 0, 0, 0));

    draw_model_details(p, detail_hi, w, h);

    // Fender lips live in fixed detail, giving each arch a crisp modern edge.
    for (const int cx : {front_cx, rear_cx}) {
        for (int a = 200; a <= 340; ++a) {
            const double rad = a * 0.017453292519943295;
            const int x = cx + static_cast<int>(std::cos(rad) * wheel_r * 1.08);
            const int y = wheel_cy + static_cast<int>(std::sin(rad) * wheel_r * 1.08);
            draw_disc(detail_hi, x, y, std::max(2, h / 240), rgba(198, 201, 204));
        }
    }

    // Independent chrome bumper layers, front left / rear right.
    const int bumper_h = std::max(10, h / 36);
    const int bumper_y = static_cast<int>(std::lround((p.rocker_y - 0.115) * h));
    front_bumper_hi.fill(static_cast<int>(0.006 * w), bumper_y,
                         static_cast<int>(0.095 * w), bumper_h, rgba(216, 221, 224));
    rear_bumper_hi.fill(static_cast<int>(0.900 * w), bumper_y,
                        static_cast<int>(0.092 * w), bumper_h, rgba(216, 221, 224));
    draw_line_hd(front_bumper_hi, static_cast<int>(0.008 * w), bumper_y,
                 static_cast<int>(0.100 * w), bumper_y, std::max(2, h / 180), rgba(247, 248, 249));
    draw_line_hd(rear_bumper_hi, static_cast<int>(0.900 * w), bumper_y,
                 static_cast<int>(0.990 * w), bumper_y, std::max(2, h / 180), rgba(247, 248, 249));

    CarArt art;
    art.id = std::string(p.id);
    art.front_x = p.front_x;
    art.rear_x = p.rear_x;
    art.wheel_y = p.wheel_y;
    art.wheel_r = p.wheel_r;
    art.body = scaled_copy(body_hi, final_w, final_h);
    art.details = scaled_copy(detail_hi, final_w, final_h);
    art.front_bumper = scaled_copy(front_bumper_hi, final_w, final_h);
    art.rear_bumper = scaled_copy(rear_bumper_hi, final_w, final_h);

    const int final_wheel_size = std::max(24,
        static_cast<int>(std::lround(2.0 * p.wheel_r * final_h)));
    art.front_wheel = make_hd_wheel(final_wheel_size);
    art.rear_wheel = make_hd_wheel(final_wheel_size);

    art.source.resize(final_w, final_h);
    art.source.clear(rgba(0, 0, 0, 0));
    art.source.blit(art.body, 0, 0);
    art.source.blit(art.front_wheel,
                    static_cast<int>(std::lround(p.front_x * final_w)) - art.front_wheel.width() / 2,
                    static_cast<int>(std::lround(p.wheel_y * final_h)) - art.front_wheel.height() / 2);
    art.source.blit(art.rear_wheel,
                    static_cast<int>(std::lround(p.rear_x * final_w)) - art.rear_wheel.width() / 2,
                    static_cast<int>(std::lround(p.wheel_y * final_h)) - art.rear_wheel.height() / 2);
    art.source.blit(art.details, 0, 0);
    art.source.blit(art.front_bumper, 0, 0);
    art.source.blit(art.rear_bumper, 0, 0);

    return art;
}

static constexpr std::array<CarProfile, 12> kHdCarProfiles{{
    // id              Fwheel Rwheel Wy    Wr     hood  trunk roof  wind  rF    rR    back  rocker door fast
    {"falcon64",      0.205, 0.785, 0.770, 0.145, 0.485,0.505,0.205,0.305,0.360,0.590,0.680,0.820,0.520,false},
    {"mustang65",     0.195, 0.785, 0.770, 0.145, 0.485,0.515,0.225,0.330,0.390,0.590,0.690,0.820,0.535,false},
    {"nova66",        0.190, 0.790, 0.770, 0.145, 0.475,0.500,0.215,0.300,0.355,0.610,0.695,0.820,0.520,false},
    {"camaro67",      0.205, 0.790, 0.775, 0.150, 0.480,0.505,0.245,0.335,0.395,0.600,0.700,0.825,0.535,false},
    {"charger68",     0.195, 0.785, 0.775, 0.150, 0.475,0.505,0.215,0.330,0.390,0.565,0.770,0.825,0.535,true},
    {"gto69",         0.210, 0.790, 0.775, 0.150, 0.470,0.505,0.235,0.330,0.390,0.605,0.715,0.825,0.535,false},
    {"roadrunner69",  0.195, 0.790, 0.775, 0.150, 0.465,0.500,0.220,0.320,0.380,0.600,0.710,0.825,0.525,false},
    {"challenger70",  0.205, 0.795, 0.780, 0.150, 0.465,0.500,0.245,0.345,0.405,0.610,0.720,0.830,0.540,false},
    {"cougar68",      0.195, 0.790, 0.775, 0.150, 0.480,0.510,0.220,0.325,0.385,0.610,0.700,0.825,0.535,false},
    {"torino69",      0.195, 0.790, 0.775, 0.150, 0.470,0.500,0.225,0.335,0.395,0.555,0.780,0.825,0.535,true},
    {"chevelle70",    0.200, 0.790, 0.780, 0.150, 0.470,0.505,0.240,0.335,0.395,0.605,0.715,0.830,0.535,false},
    {"barracuda70",   0.205, 0.800, 0.780, 0.150, 0.460,0.495,0.245,0.345,0.405,0.600,0.720,0.830,0.545,false}
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

        car_art.clear();
        car_art.reserve(kHdCarProfiles.size());
        for (const CarProfile& profile : kHdCarProfiles) {
            car_art.push_back(make_hd_car_art(profile));
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