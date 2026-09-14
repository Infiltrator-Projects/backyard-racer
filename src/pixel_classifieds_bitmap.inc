    // Classifieds are deliberately composed as a tiny bitmap page first.
    // The finished 320x180 page is then nearest-neighbour scaled to the game
    // framebuffer.  Nothing on this screen is laid out as full-resolution GUI
    // chrome: type, rules, advertisements, hover marks and footer are pixels in
    // the newspaper surface.
    static constexpr int classifieds_paper_w = 320;
    static constexpr int classifieds_paper_h = 180;
    static constexpr int classifieds_list_x = 56;
    static constexpr int classifieds_list_y = 30;
    static constexpr int classifieds_list_w = 208;
    static constexpr int classifieds_row_h = 17;

    SurfaceOwner classifieds_paper_;

    static Color paper_colour() { return rgba(229, 226, 211); }
    static Color paper_light() { return rgba(238, 235, 220); }
    static Color paper_ink() { return rgba(24, 23, 20); }
    static Color paper_rule() { return rgba(74, 71, 64); }
    static Color paper_fleck() { return rgba(166, 162, 149); }
    static Color paper_hover() { return rgba(111, 42, 35); }

    void paper_set(int x, int y, Color colour) {
        if (x < 0 || y < 0 || x >= classifieds_paper_w || y >= classifieds_paper_h) return;
        infiltratr_surface_set_pixel(&classifieds_paper_.get(), x, y, colour);
    }

    void paper_fill(int x, int y, int w, int h, Color colour) {
        infiltratr_surface_fill_rect(&classifieds_paper_.get(), x, y, w, h, colour);
    }

    void paper_rule_h(int x, int y, int w, Color colour = paper_rule()) {
        // A rule here is literally a row of pixels in the low-resolution page,
        // not a full-resolution GUI line primitive.
        for (int px = 0; px < w; ++px) paper_set(x + px, y, colour);
    }

    void paper_rule_v(int x, int y, int h, Color colour = paper_rule()) {
        for (int py = 0; py < h; ++py) paper_set(x, y + py, colour);
    }

    int paper_text_width(std::string_view value, int scale = 1) const {
        return static_cast<int>(value.size()) * 6 * scale;
    }

    void paper_text(int x, int y, std::string_view value, Color colour,
                    int scale = 1, bool bold = false, int max_width = 0) {
        int cursor = x;
        const int limit = max_width > 0 ? x + max_width : classifieds_paper_w;
        for (char ch : value) {
            const int glyph_w = 6 * scale;
            if (cursor + glyph_w > limit) break;
            const auto rows = glyph_rows(ch);
            for (int gy = 0; gy < 7; ++gy) {
                for (int gx = 0; gx < 5; ++gx) {
                    if ((rows[gy] & (1U << (4 - gx))) == 0) continue;
                    for (int sy = 0; sy < scale; ++sy) {
                        for (int sx = 0; sx < scale; ++sx) {
                            paper_set(cursor + gx * scale + sx, y + gy * scale + sy, colour);
                            if (bold) paper_set(cursor + gx * scale + sx + 1,
                                                y + gy * scale + sy, colour);
                        }
                    }
                }
            }
            cursor += glyph_w;
        }
    }

    void paper_speckle() {
        std::uint32_t state = 0x8D31A4C7U ^
            static_cast<std::uint32_t>(game.current_week() * 2654435761U);
        for (int i = 0; i < 165; ++i) {
            state = state * 1664525U + 1013904223U;
            const int x = static_cast<int>((state >> 8U) % classifieds_paper_w);
            state = state * 1664525U + 1013904223U;
            const int y = static_cast<int>((state >> 8U) % (classifieds_paper_h - 12));
            paper_set(x, y, (state & 3U) == 0U ? rgba(198, 194, 180) : paper_fleck());
        }
    }

    Rect paper_to_screen_rect(Rect p) const {
        const int x0 = p.x * width / classifieds_paper_w;
        const int y0 = p.y * height / classifieds_paper_h;
        const int x1 = (p.x + p.w) * width / classifieds_paper_w;
        const int y1 = (p.y + p.h) * height / classifieds_paper_h;
        return {x0, y0, std::max(1, x1 - x0), std::max(1, y1 - y0)};
    }

    Rect classified_ad_rect(std::size_t index) const {
        return paper_to_screen_rect({classifieds_list_x,
                                     classifieds_list_y + static_cast<int>(index) * classifieds_row_h,
                                     classifieds_list_w,
                                     classifieds_row_h});
    }

    Rect newspaper_next_week_rect() const {
        return paper_to_screen_rect({214, 169, 52, 10});
    }

    Rect newspaper_garage_rect() const {
        return paper_to_screen_rect({269, 169, 48, 10});
    }

    bool paper_hover(Rect p) const {
        return paper_to_screen_rect(p).contains(mouse_x, mouse_y);
    }

    void paper_side_ad(int x, int y, int w,
                       std::string_view heading,
                       std::string_view line1,
                       std::string_view line2,
                       bool reverse) {
        const int h = 33;
        if (reverse) {
            paper_fill(x, y, w, h - 1, paper_ink());
            paper_text(x + 2, y + 3, heading, paper_colour(), 1, true, w - 4);
            paper_text(x + 2, y + 13, line1, paper_colour(), 1, false, w - 4);
            paper_text(x + 2, y + 22, line2, paper_colour(), 1, false, w - 4);
        } else {
            paper_text(x + 2, y + 3, heading, paper_ink(), 1, true, w - 4);
            paper_text(x + 2, y + 13, line1, paper_ink(), 1, false, w - 4);
            paper_text(x + 2, y + 22, line2, paper_ink(), 1, false, w - 4);
            paper_rule_h(x, y + h - 1, w);
        }
    }

    void compose_classifieds_paper() {
        if (classifieds_paper_.empty() ||
            classifieds_paper_.width() != classifieds_paper_w ||
            classifieds_paper_.height() != classifieds_paper_h) {
            classifieds_paper_.resize(classifieds_paper_w, classifieds_paper_h);
        }

        classifieds_paper_.clear(paper_colour());
        paper_speckle();

        // Masthead and edition are deliberately small, dense bitmap print.
        paper_text(4, 3, "THE BACKYARD GAZETTE", paper_ink(), 1, true);
        const std::string edition = "WK " + std::to_string(game.current_week()) + " SAT MOTORING";
        paper_text(classifieds_paper_w - paper_text_width(edition) - 4,
                   4, edition, paper_ink(), 1, false);
        paper_rule_h(2, 13, classifieds_paper_w - 4, paper_ink());

        const std::string used = "USED CARS";
        const int used_w = paper_text_width(used);
        const int used_x = (classifieds_paper_w - used_w) / 2;
        paper_rule_h(2, 18, used_x - 6);
        paper_text(used_x, 15, used, paper_ink(), 1, true);
        paper_rule_h(used_x + used_w + 4, 18,
                     classifieds_paper_w - (used_x + used_w + 6));
        paper_text(classifieds_list_x, 23,
                   "PRIVATE SALES  AUD  KW POWER  KG MASS",
                   paper_ink(), 1, false, classifieds_list_w);

        // Narrow local adverts crowd the page exactly like cheap classifieds.
        paper_side_ad(2,   30, 50, "TYRES",   "MAGS RAD", "SAT FIT", true);
        paper_side_ad(2,   64, 50, "PAINT",   "RUST DENT", "CASH OK", false);
        paper_side_ad(2,   98, 50, "TOWING",  "LOCAL HWY", "LATE OK", true);
        paper_side_ad(2,  132, 50, "WANTED",  "OLD V8", "CASH PAID", false);

        paper_side_ad(268, 30, 50, "SPEED",   "CARBS CAM", "FORD GM", false);
        paper_side_ad(268, 64, 50, "RACING",  "SAT 8PM", "CASH PINK", true);
        paper_side_ad(268, 98, 50, "EXHAUST", "PIPES MUF", "SAME DAY", false);
        paper_side_ad(268,132, 50, "MOTORS",  "NEW STOCK", "EACH WK", true);

        const auto& cars = game.classifieds();
        static constexpr const char* short_blurbs[] = {
            "RUNS WELL", "GOOD DRIVER", "TIDY CAR", "NO SWAPS",
            "MAKE OFFER", "GOOD TYRES", "GARAGED", "VIEW AFTER 6"
        };

        const std::size_t count = std::min<std::size_t>(cars.size(), 8);
        for (std::size_t i = 0; i < count; ++i) {
            const int y = classifieds_list_y + static_cast<int>(i) * classifieds_row_h;
            const Rect paper_ad{classifieds_list_x, y, classifieds_list_w, classifieds_row_h};
            const bool hover = paper_hover(paper_ad);
            if (hover) paper_fill(classifieds_list_x, y + 1, 2, classifieds_row_h - 2, paper_hover());

            const CarSpec& car = cars[i];
            std::string first = car_display_name(car);
            if (!car.variant.empty()) first += " " + car.variant;
            paper_text(classifieds_list_x + 4, y + 1, first,
                       paper_ink(), 1, true, classifieds_list_w - 6);

            const std::string second = "$" + std::to_string(car.price) + " " +
                std::to_string(display_kw(car.horsepower)) + "KW " +
                std::to_string(display_kg(car.weight_lb)) + "KG " +
                short_blurbs[i % std::size(short_blurbs)];
            paper_text(classifieds_list_x + 4, y + 9, second,
                       paper_ink(), 1, false, classifieds_list_w - 6);
            paper_rule_h(classifieds_list_x, y + classifieds_row_h - 1,
                         classifieds_list_w, paper_rule());
        }

        // Old-game status strip: it is part of the bitmap page, not a modern UI bar.
        paper_fill(0, 168, classifieds_paper_w, 12, paper_ink());
        paper_text(4, 170, "BANK $" + std::to_string(game.cash()), paper_light(), 1, true, 76);
        if (!message.empty()) {
            paper_text(82, 170, message, paper_light(), 1, false, 126);
        }

        const Rect next_paper{214, 169, 52, 10};
        const Rect garage_paper{269, 169, 48, 10};
        if (paper_hover(next_paper)) {
            paper_fill(next_paper.x, next_paper.y, next_paper.w, next_paper.h, paper_light());
            paper_text(216, 170, "NEXT WK", paper_ink(), 1, true, 48);
        } else {
            paper_text(216, 170, "NEXT WK", paper_light(), 1, false, 48);
        }
        if (paper_hover(garage_paper)) {
            paper_fill(garage_paper.x, garage_paper.y, garage_paper.w, garage_paper.h, paper_light());
            paper_text(272, 170, "GARAGE", paper_ink(), 1, true, 44);
        } else {
            paper_text(272, 170, "GARAGE", paper_light(), 1, false, 44);
        }
    }

    void draw_classifieds_au() {
        compose_classifieds_paper();
        frame.clear(rgba(24, 23, 20));
        infiltratr_surface_blit_scaled_nearest(&frame.get(), &classifieds_paper_.get(),
                                               0, 0, width, height);
    }

    void draw() {
        switch (screen) {
            case Screen::Menu: draw_menu(); break;
            case Screen::Classifieds: draw_classifieds_au(); break;
            case Screen::Garage: draw_garage_au(); break;
            case Screen::Parts: draw_parts_au(); break;
            case Screen::Diner: draw_diner(); break;
            case Screen::Race: draw_race_au(); break;
            case Screen::Result: draw_result(); break;
            case Screen::Settings: draw_settings(); break;
        }
    }
