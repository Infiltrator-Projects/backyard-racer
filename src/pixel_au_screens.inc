    static int display_kw(int horsepower) {
        return static_cast<int>(std::lround(static_cast<double>(horsepower) * 0.745699872));
    }

    static int display_kg(int pounds) {
        return static_cast<int>(std::lround(static_cast<double>(pounds) * 0.45359237));
    }

    static int display_kmh(double mph) {
        return static_cast<int>(std::lround(mph * 1.609344));
    }

    static int display_metres(double feet) {
        return static_cast<int>(std::lround(feet * 0.3048));
    }

    void newspaper_bold(int x, int y, const std::string& value,
                        GlyphAtlas::Tone tone = GlyphAtlas::Tone::Ink,
                        int scale = 1) {
        text(x + 1, y, value, tone, scale);
        text(x, y, value, tone, scale);
    }

    void draw_newsprint_sheet() {
        const Color desk = rgba(45, 42, 36);
        const Color shadow = rgba(24, 22, 19);
        const Color paper = rgba(226, 223, 210);
        const Color paper_light = rgba(232, 229, 216);
        const Color fleck = rgba(174, 170, 158);

        frame.clear(desk);
        frame.fill(20, 22, width - 30, height - 24, shadow);
        frame.fill(12, 12, width - 32, height - 28, paper);

        // Slight stock variation and folds stop the sheet looking like a beige UI panel.
        for (int y = 16; y < height - 20; y += 41) {
            frame.fill(16, y, width - 40, 1, (y / 41) % 2 ? paper_light : rgba(219, 216, 204));
        }
        frame.fill(width / 2, 16, 1, height - 40, rgba(207, 203, 191));
        frame.fill(width / 2 + 1, 16, 1, height - 40, paper_light);

        std::uint32_t state = 0x6D2B79F5U ^ static_cast<std::uint32_t>(game.current_week() * 2654435761U);
        const int flecks = std::clamp((width * height) / 1700, 180, 620);
        for (int i = 0; i < flecks; ++i) {
            state = state * 1664525U + 1013904223U;
            const int x = 18 + static_cast<int>((state >> 8U) % static_cast<std::uint32_t>(std::max(1, width - 52)));
            state = state * 1664525U + 1013904223U;
            const int y = 18 + static_cast<int>((state >> 8U) % static_cast<std::uint32_t>(std::max(1, height - 56)));
            frame.fill(x, y, (state & 7U) == 0U ? 2 : 1, 1, fleck);
        }
    }

    int newspaper_sidebar_width() const {
        if (width < 900) return 0;
        return std::clamp(width / 7, 132, 180);
    }

    Rect classified_ad_rect(std::size_t index) const {
        const int margin = 32;
        const int gutter = 14;
        const int sidebar = newspaper_sidebar_width();
        const int centre_x = margin + (sidebar ? sidebar + gutter : 0);
        const int centre_w = width - margin * 2 - (sidebar ? (sidebar * 2 + gutter * 2) : 0);
        const int top = 126;
        const int bottom = height - 66;
        const int row_h = std::max(34, (bottom - top) / 8);
        return {centre_x, top + static_cast<int>(index) * row_h, centre_w, row_h};
    }

    Rect newspaper_next_week_rect() const {
        return {width - 360, height - 53, 150, 30};
    }

    Rect newspaper_garage_rect() const {
        return {width - 198, height - 53, 150, 30};
    }

    void newspaper_link(Rect r, const std::string& label) {
        const bool hover = r.contains(mouse_x, mouse_y);
        if (hover) frame.fill(r.x, r.y, r.w, r.h, rgba(101, 36, 31));
        text_center(r, label,
                    hover ? GlyphAtlas::Tone::Gold : GlyphAtlas::Tone::Pale,
                    1);
        if (!hover) frame.fill(r.x + 6, r.y + r.h - 4, r.w - 12, 1, rgba(196, 191, 178));
    }

    void newspaper_sidebar_ad(Rect r, const std::string& heading,
                              const std::string& line1, const std::string& line2,
                              bool reverse) {
        const Color ink = rgba(27, 26, 23);
        const Color rule = rgba(78, 74, 67);
        if (reverse) {
            frame.fill(r.x, r.y, r.w, r.h, ink);
            newspaper_bold(r.x + 8, r.y + 8, heading, GlyphAtlas::Tone::Pale, 1);
            text(r.x + 8, r.y + 27, line1, GlyphAtlas::Tone::Pale, 1);
            text(r.x + 8, r.y + 43, line2, GlyphAtlas::Tone::Pale, 1);
        } else {
            frame.fill(r.x, r.y + r.h - 1, r.w, 1, rule);
            newspaper_bold(r.x + 7, r.y + 7, heading, GlyphAtlas::Tone::Ink, 1);
            text(r.x + 7, r.y + 26, line1, GlyphAtlas::Tone::Ink, 1);
            text(r.x + 7, r.y + 42, line2, GlyphAtlas::Tone::Ink, 1);
        }
    }

    void draw_newspaper_sidebars(int top, int bottom) {
        const int margin = 32;
        const int gutter = 14;
        const int sidebar = newspaper_sidebar_width();
        if (sidebar == 0) return;
        const int centre_w = width - margin * 2 - sidebar * 2 - gutter * 2;
        const int left_x = margin;
        const int right_x = margin + sidebar + gutter + centre_w + gutter;
        const int available_h = bottom - top;
        const int ad_h = std::max(58, available_h / 4);

        newspaper_sidebar_ad({left_x, top, sidebar, ad_h - 4},
                             "REDLINE TYRES", "BIAS PLY - RADIALS", "FITTED SATURDAY", true);
        newspaper_sidebar_ad({left_x, top + ad_h, sidebar, ad_h - 4},
                             "PANEL & PAINT", "RUST - DENTS - RESPRAYS", "CASH JOBS WELCOME", false);
        newspaper_sidebar_ad({left_x, top + ad_h * 2, sidebar, ad_h - 4},
                             "NIGHT TOWING", "LOCAL & HIGHWAY", "AFTER HOURS OK", true);
        newspaper_sidebar_ad({left_x, top + ad_h * 3, sidebar, bottom - (top + ad_h * 3)},
                             "WANTED", "OLD V8 PARTS", "CASH PAID", false);

        newspaper_sidebar_ad({right_x, top, sidebar, ad_h - 4},
                             "SPEED SHOP", "CARBS - CAMS - HEADERS", "FORD GM MOPAR", false);
        newspaper_sidebar_ad({right_x, top + ad_h, sidebar, ad_h - 4},
                             "RACE NIGHT", "SATURDAY 8 PM", "CASH & PINKS", true);
        newspaper_sidebar_ad({right_x, top + ad_h * 2, sidebar, ad_h - 4},
                             "EXHAUST CENTRE", "PIPES - MUFFLERS", "SAME DAY FITTING", false);
        newspaper_sidebar_ad({right_x, top + ad_h * 3, sidebar, bottom - (top + ad_h * 3)},
                             "MOTOR TRADER", "FRESH STOCK WEEKLY", "TURN THE PAGE", true);
    }

    void draw_classifieds_au() {
        const Color ink = rgba(27, 26, 23);
        const Color rule = rgba(83, 78, 69);
        const Color hover_ink = rgba(128, 50, 42);
        draw_newsprint_sheet();

        newspaper_bold(34, 24, "THE BACKYARD GAZETTE", GlyphAtlas::Tone::Ink, 3);
        const std::string edition = "WEEK " + std::to_string(game.current_week()) + " - SATURDAY MOTORING";
        text(width - glyphs.width(edition, 1) - 38, 32, edition, GlyphAtlas::Tone::Ink, 1);
        frame.fill(32, 62, width - 64, 3, ink);
        frame.fill(32, 68, width - 64, 1, rule);

        frame.fill(32, 77, width - 64, 30, ink);
        text_center({32, 77, width - 64, 30}, "MOTORING CLASSIFIEDS - USED CARS",
                    GlyphAtlas::Tone::Pale, 2);
        text(34, 111, "PRIVATE SALES - PRICES AUD - POWER kW - MASS kg - CLICK A LISTING TO BUY",
             GlyphAtlas::Tone::Ink, 1);

        const int content_top = 126;
        const int content_bottom = height - 66;
        draw_newspaper_sidebars(content_top, content_bottom);

        const auto& cars = game.classifieds();
        static constexpr const char* blurbs[] = {
            "LOCAL CAR. RUNS WELL. INSPECTION WELCOME.",
            "STRAIGHT BODY. GOOD DRIVER. READY TO GO.",
            "TIDY FOR AGE. HONEST OLD CAR. NO SWAPS.",
            "WEEKEND CAR. STARTS EASY. SELLING AS IS.",
            "ORIGINAL TRIM. ROADWORTHY PROJECT. MAKE OFFER.",
            "GOOD TYRES. DRIVES WELL. FIRST TO SEE WILL BUY.",
            "OLDER RESTORATION. PRESENTABLE. INSPECTION OK.",
            "PRIVATE SALE. GARAGED. VIEW AFTER WORK."
        };

        for (std::size_t i = 0; i < cars.size(); ++i) {
            const Rect ad = classified_ad_rect(i);
            const bool hover = ad.contains(mouse_x, mouse_y);
            if (hover) {
                frame.fill(ad.x, ad.y, ad.w, ad.h, rgba(214, 210, 196));
                frame.fill(ad.x, ad.y, 4, ad.h, hover_ink);
            }
            frame.fill(ad.x, ad.y + ad.h - 1, ad.w, 1, rule);

            const CarSpec& car = cars[i];
            const int row_h = ad.h;
            const int line1 = ad.y + 3;
            const int line2 = ad.y + 16;
            const int line3 = ad.y + 29;
            const int line4 = ad.y + 42;

            newspaper_bold(ad.x + 7, line1, car_display_name(car), GlyphAtlas::Tone::Ink, 1);
            const std::string detail = (car.variant.empty() ? std::string("STANDARD ENGINE") : car.variant) +
                "   $" + std::to_string(car.price) + "   " +
                std::to_string(display_kw(car.horsepower)) + " kW   " +
                std::to_string(display_kg(car.weight_lb)) + " kg";
            text(ad.x + 7, line2, detail, GlyphAtlas::Tone::Ink, 1);

            const std::string ref = "REF BG" + std::to_string(game.current_week()) + "-" + std::to_string(i + 1);
            text(ad.x + 7, line3, ref, GlyphAtlas::Tone::Ink, 1);
            if (row_h >= 50) {
                text(ad.x + 100, line3, blurbs[i % std::size(blurbs)], GlyphAtlas::Tone::Ink, 1);
            }

            if (CarArt* art = art_for(car.id)) {
                const int preview_w = std::clamp(row_h * 2, 70, 118);
                const int preview_h = std::max(1, art->source.height() * preview_w / art->source.width());
                SurfaceOwner preview = scaled_copy(art->source, preview_w, preview_h);
                frame.blit(preview, ad.x + ad.w - preview_w - 5,
                           ad.y + std::max(1, (ad.h - preview_h) / 2));
            } else if (row_h >= 58) {
                text(ad.x + 7, line4, "CALL AFTER 6 PM - INSPECTION BY ARRANGEMENT",
                     GlyphAtlas::Tone::Ink, 1);
            }
        }

        const Rect next_week = newspaper_next_week_rect();
        const Rect garage = newspaper_garage_rect();
        frame.fill(12, height - 58, width - 32, 46, ink);
        newspaper_bold(30, height - 46,
                       "BANKROLL $" + std::to_string(game.cash()), GlyphAtlas::Tone::Pale, 1);
        if (!message.empty()) {
            text(165, height - 46, message, GlyphAtlas::Tone::Pale, 1);
        }
        newspaper_link(next_week, "NEXT WEEK >");
        newspaper_link(garage, "GARAGE >");
    }

    void draw_garage_au() {
        if (!garage_scaled.empty()) infiltratr_surface_copy(&frame.get(), &garage_scaled.get());
        else frame.clear(rgba(45, 42, 38));
        status("BACKYARD GARAGE");
        const bool moving = motion != Motion::Idle;
        if (displayed) {
            const OwnedCar& car = *displayed;
            panel({26, 82, 760, 160});
            text(44, 101, car_display_name(car.base), GlyphAtlas::Tone::Gold, 2);
            text(44, 128,
                 (car.base.variant.empty() ? std::string("ENGINE") : car.base.variant) +
                 "  " + std::to_string(display_kw(car.horsepower())) + " kW" +
                 "  " + std::to_string(display_kg(car.base.weight_lb)) + " kg",
                 GlyphAtlas::Tone::Pale, 2);
            text(44, 154,
                 "PAINT " + car.paint.name +
                 "  CONDITION " + std::to_string(car.condition) + "%",
                 GlyphAtlas::Tone::Pale, 2);
            text(44, 180,
                 "SELL $" + std::to_string(car.resale_value()) +
                 "  REPAIR $" + std::to_string(car.repair_cost()) +
                 "  SPARES " + std::to_string(game.spare_parts().size()),
                 GlyphAtlas::Tone::Pale, 2);
            button({38, 204, 138, 34}, "REPAIR", !moving && car.repair_cost() > 0);
            button({190, 204, 138, 34}, "SELL CAR", !moving);
            const std::string front_bumper_action =
                car.front_bumper_installed ? "FRONT OFF $50" : "FRONT ON $50";
            const std::string rear_bumper_action =
                car.rear_bumper_installed ? "REAR OFF $50" : "REAR ON $50";
            button({342, 204, 190, 34}, front_bumper_action,
                   !moving && game.cash() >= GameState::bumper_change_cost());
            button({546, 204, 190, 34}, rear_bumper_action,
                   !moving && game.cash() >= GameState::bumper_change_cost());
            const int x = static_cast<int>(std::lround(moving ? car_x : parked_x(car)));
            draw_car(car, x, height * 86 / 100, wheel_angle);
        } else {
            panel({30, 88, 410, 92});
            text(48, 112, "EMPTY GARAGE", GlyphAtlas::Tone::Gold, 3);
            text(48, 146, "BUY A CAR FROM THE PAPER.", GlyphAtlas::Tone::Pale, 2);
        }
        if (moving) {
            panel({width / 2 - 150, height - 128, 300, 40});
            text_center({width / 2 - 150, height - 128, 300, 40},
                        motion == Motion::Exiting ? "REVERSING OUT..." : "DRIVING IN...",
                        GlyphAtlas::Tone::Gold, 2);
        }
        button({30, height - 57, 174, 38}, "CLASSIFIEDS", !moving);
        button({216, height - 57, 154, 38}, "PARTS / TUNE", !moving && game.active_car());
        button({382, height - 57, 148, 38}, "DRIVE-IN", !moving && game.active_car());
        button({542, height - 57, 150, 38}, "NEXT CAR", !moving && game.garage().size() > 1);
        button({width - 180, height - 57, 150, 38}, "MAIN MENU", !moving);
        text(32, height - 82, message, GlyphAtlas::Tone::Pale, 2);
    }

    void draw_parts_au() {
        frame.clear(rgba(42, 39, 34));
        status("PARTS AND TUNING");
        panel({28, 86, width - 56, height - 166});
        if (!game.active_car()) {
            text(58, 118, "BUY A CAR FIRST.", GlyphAtlas::Tone::Gold, 3);
        } else {
            const OwnedCar& car = *game.active_car();
            text(58, 100, car_display_name(car.base), GlyphAtlas::Tone::Gold, 2);
            text(58, 124,
                 "CURRENT ENGINE: " + (car.base.variant.empty() ? std::string("UNKNOWN") : car.base.variant) +
                 "  " + std::to_string(display_kw(car.base.horsepower)) + " kW",
                 GlyphAtlas::Tone::Pale, 1);

            const int left_x = 54;
            const int right_x = width / 2 + 20;
            const int left_w = width / 2 - 82;
            const int right_w = width - right_x - 54;
            const int row_h = 34;
            const int first_row_y = 174;
            text(left_x, 151, "PERFORMANCE PARTS", GlyphAtlas::Tone::Gold, 2);
            text(right_x, 151, "COMPATIBLE ENGINES", GlyphAtlas::Tone::Gold, 2);

            const auto& parts = game.parts_catalog();
            for (std::size_t i = 0; i < parts.size(); ++i) {
                const int y = first_row_y + static_cast<int>(i) * row_h;
                if (y + row_h > height - 132) break;
                const Rect row{left_x, y, left_w, row_h - 3};
                if (row.contains(mouse_x, mouse_y)) frame.blend(row.x, row.y, row.w, row.h, rgba(98, 32, 28, 120));
                text(row.x + 8, row.y + 7,
                     parts[i].name + "  $" + std::to_string(parts[i].price),
                     GlyphAtlas::Tone::Pale, 1);
            }

            const auto engines = game.compatible_engines();
            for (std::size_t i = 0; i < engines.size(); ++i) {
                const int y = first_row_y + static_cast<int>(i) * row_h;
                if (y + row_h > height - 132) break;
                const Rect row{right_x, y, right_w, row_h - 3};
                if (row.contains(mouse_x, mouse_y)) frame.blend(row.x, row.y, row.w, row.h, rgba(98, 32, 28, 120));
                text(row.x + 8, row.y + 7,
                     engines[i].name + "  " + std::to_string(display_kw(engines[i].horsepower)) +
                     " kW  $" + std::to_string(engines[i].price),
                     GlyphAtlas::Tone::Pale, 1);
            }

            if (!game.spare_parts().empty()) {
                const int y = height - 116;
                text(58, y, "SPARE BIN - CLICK: " + game.spare_parts().front().name,
                     GlyphAtlas::Tone::Gold, 1);
            }
        }
        button({30, height - 57, 150, 38}, "GARAGE");
        text(210, height - 47, message, GlyphAtlas::Tone::Pale, 2);
    }

    void draw_race_au() {
        frame.clear(rgba(24, 29, 33));
        status("QUARTER MILE");
        const int road_y = 250;
        frame.fill(0, road_y, width, 300, rgba(53, 54, 55));
        frame.fill(0, road_y + 138, width, 4, rgba(230, 220, 190));
        for (int x = 0; x < width; x += 90) frame.fill(x, road_y + 70, 45, 4, rgba(205, 195, 165));

        const double player_progress = std::clamp(race.distance_ft() / 1320.0, 0.0, 1.0);
        const double opponent_progress = std::clamp(race.opponent_distance_ft() / 1320.0, 0.0, 1.0);
        const int start_x = 80;
        const int finish_x = width - 100;
        const int player_x = start_x + static_cast<int>((finish_x - start_x) * player_progress);
        const int opponent_x = start_x + static_cast<int>((finish_x - start_x) * opponent_progress);
        frame.fill(player_x - 28, road_y + 178, 56, 24, rgba(191, 54, 46));
        frame.fill(opponent_x - 28, road_y + 68, 56, 24, rgba(67, 95, 139));
        frame.fill(finish_x, road_y, 5, 300, rgba(245, 240, 220));

        panel({24, 78, 590, 138});
        text(42, 94, race.green() ? "GREEN" : "STAGE",
             race.green() ? GlyphAtlas::Tone::Gold : GlyphAtlas::Tone::Red, 3);
        text(42, 132,
             "GEAR " + std::to_string(race.gear()) +
             "  RPM " + std::to_string(race.rpm()) +
             "  " + std::to_string(display_kmh(race.speed_mph())) + " km/h",
             GlyphAtlas::Tone::Pale, 2);
        text(42, 164,
             "DIST " + std::to_string(display_metres(race.distance_ft())) +
             " m  SPACE THROTTLE  UP/DOWN SHIFT",
             GlyphAtlas::Tone::Pale, 2);
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