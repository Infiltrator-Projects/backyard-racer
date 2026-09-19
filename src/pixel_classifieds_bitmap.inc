    // Classifieds are composed as a deliberately low-resolution newspaper
    // page first, then nearest-neighbour scaled to the game framebuffer.  The
    // 640x360 native page keeps the print visibly raster-authored without
    // turning 5x7 glyphs into oversized terminal lettering on a 1280x720 host.
    static constexpr int classifieds_paper_w = 640;
    static constexpr int classifieds_paper_h = 360;
    static constexpr int classifieds_list_x = 116;
    static constexpr int classifieds_list_y = 58;
    static constexpr int classifieds_list_w = 408;
    static constexpr int classifieds_row_h = 32;

    SurfaceOwner classifieds_paper_;

    static Color paper_colour() { return rgba(232, 229, 216); }
    static Color paper_light() { return rgba(242, 239, 226); }
    static Color paper_ink() { return rgba(29, 28, 24); }
    static Color paper_rule() { return rgba(94, 90, 80); }
    static Color paper_fleck() { return rgba(181, 176, 160); }
    static Color paper_hover() { return rgba(116, 48, 39); }

    void paper_set(int x, int y, Color colour) {
        if (x < 0 || y < 0 || x >= classifieds_paper_w || y >= classifieds_paper_h) return;
        infiltratr_surface_set_pixel(&classifieds_paper_.get(), x, y, colour);
    }

    void paper_fill(int x, int y, int w, int h, Color colour) {
        infiltratr_surface_fill_rect(&classifieds_paper_.get(), x, y, w, h, colour);
    }

    void paper_rule_h(int x, int y, int w, Color colour = paper_rule()) {
        for (int px = 0; px < w; ++px) paper_set(x + px, y, colour);
    }

    void paper_dotted_rule_h(int x, int y, int w, Color colour = paper_rule()) {
        for (int px = 0; px < w; px += 3) {
            paper_set(x + px, y, colour);
            if (px + 1 < w) paper_set(x + px + 1, y, colour);
        }
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
                            if (bold) {
                                paper_set(cursor + gx * scale + sx + 1,
                                          y + gy * scale + sy, colour);
                            }
                        }
                    }
                }
            }
            cursor += glyph_w;
        }
    }

    std::string paper_money(int value) const {
        std::string digits = std::to_string(std::max(0, value));
        for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(digits.size()) - 3; i > 0; i -= 3) {
            digits.insert(static_cast<std::size_t>(i), 1, ',');
        }
        return "$" + digits;
    }

    void paper_speckle() {
        std::uint32_t state = 0x8D31A4C7U ^
            static_cast<std::uint32_t>(game.current_week() * 2654435761U);
        for (int i = 0; i < 420; ++i) {
            state = state * 1664525U + 1013904223U;
            const int x = static_cast<int>((state >> 8U) % classifieds_paper_w);
            state = state * 1664525U + 1013904223U;
            const int y = static_cast<int>((state >> 8U) % (classifieds_paper_h - 24));
            if ((state & 7U) < 5U) continue;
            paper_set(x, y, (state & 8U) ? paper_fleck() : rgba(202, 198, 184));
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
        return paper_to_screen_rect({
            classifieds_list_x,
            classifieds_list_y + static_cast<int>(index) * classifieds_row_h,
            classifieds_list_w,
            classifieds_row_h
        });
    }

    Rect newspaper_next_week_rect() const {
        return paper_to_screen_rect({486, 338, 72, 18});
    }

    Rect newspaper_garage_rect() const {
        return paper_to_screen_rect({564, 338, 72, 18});
    }

    Rect newspaper_article_back_rect() const {
        return paper_to_screen_rect({476, 338, 78, 18});
    }

    Rect newspaper_article_buy_rect() const {
        return paper_to_screen_rect({558, 338, 78, 18});
    }

    bool paper_hover(Rect p) const {
        return paper_to_screen_rect(p).contains(mouse_x, mouse_y);
    }

    void paper_pointer(int x, int y) {
        const Color c = paper_hover();
        paper_set(x, y + 1, c);
        paper_set(x + 1, y + 2, c);
        paper_set(x + 2, y + 3, c);
        paper_set(x + 3, y + 4, c);
        paper_set(x + 2, y + 5, c);
        paper_set(x + 1, y + 6, c);
        paper_set(x, y + 7, c);
    }

    void paper_side_ad(int x, int y, int w,
                       std::string_view heading,
                       std::string_view line1,
                       std::string_view line2,
                       std::string_view line3,
                       bool reverse_heading = false) {
        const int h = 62;
        if (reverse_heading) {
            paper_fill(x, y, w, 11, paper_ink());
            paper_text(x + 3, y + 2, heading, paper_light(), 1, true, w - 6);
        } else {
            paper_text(x + 3, y + 2, heading, paper_ink(), 1, true, w - 6);
            paper_dotted_rule_h(x + 2, y + 11, w - 4);
        }

        paper_text(x + 3, y + 17, line1, paper_ink(), 1, false, w - 6);
        paper_text(x + 3, y + 28, line2, paper_ink(), 1, false, w - 6);
        paper_text(x + 3, y + 39, line3, paper_ink(), 1, false, w - 6);
        paper_dotted_rule_h(x + 2, y + h - 2, w - 4);
    }

    void compose_classifieds_paper() {
        if (classifieds_paper_.empty() ||
            classifieds_paper_.width() != classifieds_paper_w ||
            classifieds_paper_.height() != classifieds_paper_h) {
            classifieds_paper_.resize(classifieds_paper_w, classifieds_paper_h);
        }

        classifieds_paper_.clear(paper_colour());
        paper_speckle();

        // Masthead: authored as bitmap print, but no longer four-times blown up.
        paper_text(8, 6, "THE BACKYARD GAZETTE", paper_ink(), 2, true);
        const std::string edition =
            "WEEK " + std::to_string(game.current_week()) + "  SATURDAY MOTORING";
        paper_text(classifieds_paper_w - paper_text_width(edition) - 10,
                   10, edition, paper_ink(), 1, false);
        paper_rule_h(6, 27, classifieds_paper_w - 12, paper_ink());
        paper_dotted_rule_h(6, 30, classifieds_paper_w - 12, paper_rule());

        const std::string used = "USED CARS";
        const int used_w = paper_text_width(used);
        const int used_x = (classifieds_paper_w - used_w) / 2;
        paper_dotted_rule_h(116, 40, used_x - 124);
        paper_text(used_x, 36, used, paper_ink(), 1, true);
        paper_dotted_rule_h(used_x + used_w + 8, 40,
                            524 - (used_x + used_w + 8));
        paper_text(classifieds_list_x + 4, 47,
                   "PRIVATE SALES - PRICES AUD - POWER KW - MASS KG",
                   paper_ink(), 1, false, classifieds_list_w - 8);

        // Side advertisements stay dense and cheap-looking, but text is given
        // enough room to read instead of being clipped into fragments.
        paper_side_ad(4,   58, 106, "REDLINE TYRES", "MAGS & RADIALS", "SATURDAY FITTING",
                      "CASH WELCOME", false);
        paper_side_ad(4,  123, 106, "PANEL & PAINT", "RUST & DENTS", "QUICK QUOTES",
                      "OLD CARS OK", false);
        paper_side_ad(4,  188, 106, "NIGHT TOWING", "LOCAL / HIGHWAY", "AFTER HOURS",
                      "CALL ANY TIME", true);
        paper_side_ad(4,  253, 106, "WANTED", "OLD V8 PARTS", "FORD GM MOPAR",
                      "CASH PAID", false);

        paper_side_ad(530, 58, 106, "SPEED SHOP", "CARBS & CAMS", "IGNITION / TUNE",
                      "FORD GM MOPAR", false);
        paper_side_ad(530,123, 106, "RACE NIGHT", "SATURDAY 8PM", "CASH / PINKS",
                      "INDUSTRIAL RD", true);
        paper_side_ad(530,188, 106, "EXHAUST CENTRE", "PIPES & MUFFLERS", "SAME DAY FIT",
                      "OPEN SATURDAY", false);
        paper_side_ad(530,253, 106, "MOTOR TRADER", "FRESH STOCK", "EVERY WEEK",
                      "PRIVATE SALES", false);

        const auto& cars = game.classifieds();
        static constexpr const char* short_blurbs[] = {
            "RUNS WELL", "GOOD DRIVER", "TIDY FOR AGE", "NO SWAPS",
            "MAKE OFFER", "GOOD TYRES", "GARAGED", "VIEW AFTER 6"
        };
        static constexpr const char* sale_notes[] = {
            "PRIVATE SALE. INSPECTION WELCOME.",
            "STRAIGHT BODY. STARTS EASY.",
            "HONEST OLD CAR. SELLING AS IS.",
            "WEEKEND CAR. ROADWORTHY PROJECT.",
            "ORIGINAL TRIM. PHONE AFTER WORK.",
            "DRIVES WELL. FIRST TO SEE BUYS.",
            "OLDER RESTORATION. PRESENTABLE.",
            "LOCAL CAR. INSPECTION BY ARRANGEMENT."
        };

        const std::size_t count = std::min<std::size_t>(cars.size(), 8);
        for (std::size_t i = 0; i < count; ++i) {
            const int y = classifieds_list_y + static_cast<int>(i) * classifieds_row_h;
            const Rect paper_ad{classifieds_list_x, y, classifieds_list_w, classifieds_row_h};
            const bool hover = paper_hover(paper_ad);
            if (hover) paper_pointer(classifieds_list_x + 1, y + 10);

            const CarSpec& car = cars[i];
            std::string first = car_display_name(car);
            if (!car.variant.empty()) first += " - " + car.variant;
            paper_text(classifieds_list_x + 10, y + 2, first,
                       paper_ink(), 1, false, classifieds_list_w - 14);

            const std::string second =
                paper_money(car.price) + "   " +
                std::to_string(display_kw(car.horsepower)) + "KW   " +
                std::to_string(display_kg(car.weight_lb)) + "KG   " +
                short_blurbs[i % std::size(short_blurbs)];
            paper_text(classifieds_list_x + 10, y + 12, second,
                       paper_ink(), 1, false, classifieds_list_w - 14);

            paper_text(classifieds_list_x + 10, y + 22,
                       sale_notes[i % std::size(sale_notes)],
                       paper_rule(), 1, false, classifieds_list_w - 14);
            paper_dotted_rule_h(classifieds_list_x, y + classifieds_row_h - 1,
                                classifieds_list_w, paper_rule());
        }

        // Footer is a single old-game status strip, not a row of modern buttons.
        paper_fill(0, 336, classifieds_paper_w, 24, paper_ink());
        paper_text(8, 343, "BANKROLL " + paper_money(game.cash()),
                   paper_light(), 1, true, 126);
        if (!message.empty()) {
            paper_text(154, 343, message, paper_light(), 1, false, 320);
        }

        const Rect next_paper{486, 338, 72, 18};
        const Rect garage_paper{564, 338, 72, 18};
        const Color next_tone = paper_hover(next_paper) ? rgba(223, 111, 83) : paper_light();
        const Color garage_tone = paper_hover(garage_paper) ? rgba(223, 111, 83) : paper_light();
        paper_text(490, 343, "NEXT WEEK", next_tone, 1, paper_hover(next_paper), 66);
        paper_text(568, 343, "GARAGE", garage_tone, 1, paper_hover(garage_paper), 64);
    }

    void draw_classifieds_au() {
        compose_classifieds_paper();
        frame.clear(rgba(24, 23, 20));
        infiltratr_surface_blit_scaled_nearest(&frame.get(), &classifieds_paper_.get(),
                                               0, 0, width, height);
    }

    void compose_classified_article_paper() {
        if (!selected_classified_index ||
            *selected_classified_index >= game.classifieds().size()) {
            selected_classified_index.reset();
            compose_classifieds_paper();
            return;
        }

        if (classifieds_paper_.empty() ||
            classifieds_paper_.width() != classifieds_paper_w ||
            classifieds_paper_.height() != classifieds_paper_h) {
            classifieds_paper_.resize(classifieds_paper_w, classifieds_paper_h);
        }

        classifieds_paper_.clear(paper_colour());
        paper_speckle();

        const std::size_t index = *selected_classified_index;
        const CarSpec& car = game.classifieds()[index];

        paper_text(8, 6, "THE BACKYARD GAZETTE", paper_ink(), 2, true);
        const std::string edition =
            "WEEK " + std::to_string(game.current_week()) + "  MOTORING CLASSIFIED";
        paper_text(classifieds_paper_w - paper_text_width(edition) - 10,
                   10, edition, paper_ink(), 1, false);
        paper_rule_h(6, 27, classifieds_paper_w - 12, paper_ink());
        paper_dotted_rule_h(6, 30, classifieds_paper_w - 12, paper_rule());

        paper_text(22, 40, car_display_name(car), paper_ink(), 2, true, 596);
        if (!car.variant.empty()) {
            paper_text(24, 60, car.variant, paper_rule(), 1, true, 592);
        }

        // Newspaper photo: the same model-specific master used by the garage.
        paper_fill(22, 82, 330, 132, paper_light());
        paper_rule_h(22, 82, 330, paper_rule());
        paper_rule_h(22, 213, 330, paper_rule());
        paper_rule_v(22, 82, 132, paper_rule());
        paper_rule_v(351, 82, 132, paper_rule());
        if (CarArt* art = art_for(car.id)) {
            const int photo_w = 292;
            const int photo_h = std::max(1, art->source.height() * photo_w / art->source.width());
            classifieds_paper_.blit_scaled(
                art->source,
                41, 102 + std::max(0, (88 - photo_h) / 2),
                photo_w, photo_h);
        } else {
            paper_text(84, 139, "MODEL PHOTO UNAVAILABLE", paper_rule(), 1, true, 220);
        }
        paper_text(30, 198, "PHOTO SUPPLIED BY SELLER", paper_rule(), 1, false, 310);

        paper_text(376, 86, "ASKING PRICE", paper_ink(), 1, true);
        paper_text(376, 101, paper_money(car.price), paper_ink(), 2, true);
        paper_dotted_rule_h(374, 124, 242);
        paper_text(376, 132, "ENGINE", paper_ink(), 1, true);
        paper_text(376, 145,
                   car.variant.empty() ? std::string("STANDARD ENGINE") : car.variant,
                   paper_ink(), 1, false, 238);
        paper_text(376, 164,
                   std::to_string(display_kw(car.horsepower)) + " KW",
                   paper_ink(), 1, false, 112);
        paper_text(490, 164,
                   std::to_string(display_kg(car.weight_lb)) + " KG",
                   paper_ink(), 1, false, 118);
        paper_dotted_rule_h(374, 181, 242);
        const std::string ref =
            "REF BG" + std::to_string(game.current_week()) + "-" + std::to_string(index + 1);
        paper_text(376, 190, ref, paper_rule(), 1, true, 238);

        paper_rule_h(22, 228, 594, paper_ink());
        paper_text(22, 237, "SELLER'S NOTES", paper_ink(), 1, true);
        static constexpr const char* article_notes[] = {
            "RUNS WELL AND PRESENTS HONESTLY FOR ITS AGE. INSPECTION WELCOME.",
            "GOOD DRIVER. STRAIGHT OLD CAR. SELLING TO MAKE ROOM IN THE SHED.",
            "TIDY FOR AGE WITH ORIGINAL CHARACTER. NO SWAPS, CASH SALE PREFERRED.",
            "WEEKEND CAR THAT STARTS EASILY. SOLD AS INSPECTED.",
            "OLDER CAR WITH PRESENTABLE TRIM. VIEWING BY ARRANGEMENT.",
            "DRIVES WELL WITH GOOD TYRES. FIRST SERIOUS BUYER TAKES IT.",
            "OLDER RESTORATION, STILL PRESENTABLE. COME AND LOOK BEFORE MAKING AN OFFER.",
            "LOCAL GARAGED CAR. INSPECTION AFTER WORK OR SATURDAY MORNING."
        };
        paper_text(22, 253, article_notes[index % std::size(article_notes)],
                   paper_ink(), 1, false, 594);
        paper_text(22, 270,
                   "PRIVATE SALE - INSPECT BEFORE PURCHASE - PRICE SHOWN IN AUD",
                   paper_rule(), 1, false, 594);

        paper_fill(0, 336, classifieds_paper_w, 24, paper_ink());
        paper_text(8, 343, "BANKROLL " + paper_money(game.cash()),
                   paper_light(), 1, true, 150);
        if (!message.empty()) {
            paper_text(166, 343, message, paper_light(), 1, false, 300);
        }

        const Rect back_paper{476, 338, 78, 18};
        const Rect buy_paper{558, 338, 78, 18};
        const Color back_tone = paper_hover(back_paper) ? rgba(223, 111, 83) : paper_light();
        const bool can_buy = game.cash() >= car.price;
        const Color buy_tone = paper_hover(buy_paper) && can_buy
            ? rgba(223, 111, 83)
            : (can_buy ? paper_light() : rgba(130, 126, 116));
        paper_text(480, 343, "BACK", back_tone, 1, paper_hover(back_paper), 70);
        paper_text(562, 343, "BUY CAR", buy_tone, 1,
                   paper_hover(buy_paper) && can_buy, 70);
    }

    void draw_classified_article_au() {
        compose_classified_article_paper();
        frame.clear(rgba(24, 23, 20));
        infiltratr_surface_blit_scaled_nearest(&frame.get(), &classifieds_paper_.get(),
                                               0, 0, width, height);
    }

    void draw() {
        switch (screen) {
            case Screen::Menu: draw_menu(); break;
            case Screen::Classifieds: draw_classifieds_au(); break;
            case Screen::ClassifiedArticle: draw_classified_article_au(); break;
            case Screen::Garage: draw_garage_au(); break;
            case Screen::Parts: draw_parts_au(); break;
            case Screen::Diner: draw_diner(); break;
            case Screen::Race: draw_race_au(); break;
            case Screen::Result: draw_result(); break;
            case Screen::Settings: draw_settings(); break;
        }
    }
