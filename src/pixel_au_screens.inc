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

    void newspaper_action(Rect r, const std::string& label) {
        const bool hover = r.contains(mouse_x, mouse_y);
        frame.fill(r.x, r.y, r.w, r.h,
                   hover ? rgba(248, 241, 214) : rgba(236, 229, 202));
        outline(frame.get(), r.x, r.y, r.w, r.h,
                hover ? rgba(38, 35, 31) : rgba(78, 71, 58), hover ? 2 : 1);
        text_center(r, label, GlyphAtlas::Tone::Dark, 2);
    }

    void draw_classifieds_au() {
        const Color edge = rgba(194, 184, 150);
        const Color paper = rgba(239, 233, 208);
        const Color ink = rgba(49, 45, 38);
        const Color rule = rgba(91, 82, 67);
        frame.clear(edge);
        frame.fill(18, 18, width - 36, height - 36, paper);
        outline(frame.get(), 18, 18, width - 36, height - 36, ink, 2);

        // Newspaper masthead and section rules.  The classifieds are deliberately
        // text-led rather than a modern card grid: dense copy, thin rules and two
        // columns like an actual local paper's motoring section.
        text(46, 31, "THE BACKYARD GAZETTE", GlyphAtlas::Tone::Dark, 3);
        const std::string edition = "WEEK " + std::to_string(game.current_week()) + "  MOTORING CLASSIFIEDS";
        text(std::max(560, width - glyphs.width(edition, 1) - 48), 39,
             edition, GlyphAtlas::Tone::Dark, 1);
        frame.fill(46, 68, width - 92, 2, ink);
        frame.fill(46, 73, width - 92, 1, rule);
        text_center({width / 2 - 160, 78, 320, 27}, "USED CARS", GlyphAtlas::Tone::Dark, 2);
        text(46, 91, "LOCAL SALES - POWER kW - WEIGHT kg - CLICK AN AD TO BUY",
             GlyphAtlas::Tone::Dark, 1);

        const auto& cars = game.classifieds();
        const int gap = 14;
        const int left = 48;
        const int top = 112;
        const int card_w = (width - 110) / 2;
        const int card_h = 112;
        const int centre_x = left + card_w + gap / 2;
        frame.fill(centre_x, top - 5, 1, card_h * 4 + 4, rule);

        static constexpr const char* blurbs[] = {
            "PRIVATE SALE. RUNS WELL. INSPECTION WELCOME.",
            "STRAIGHT BODY. GOOD DRIVER. READY TO GO.",
            "CLEAN INTERIOR. HONEST OLD CAR. NO SWAPS.",
            "WEEKEND CAR. STARTS EASY. SELLING AS IS.",
            "ORIGINAL TRIM. ROADWORTHY PROJECT. MAKE OFFER.",
            "GOOD TYRES. DRIVES WELL. FIRST TO SEE WILL BUY.",
            "OLDER RESTORATION. PRESENTABLE AND MECHANICALLY SOUND.",
            "LOCAL CAR. TIDY FOR AGE. INSPECTIONS AFTER WORK."
        };

        for (std::size_t i = 0; i < cars.size(); ++i) {
            const int col = static_cast<int>(i % 2);
            const int row = static_cast<int>(i / 2);
            const Rect ad{left + col * (card_w + gap), top + row * (card_h + 10), card_w, card_h};
            const bool hover = ad.contains(mouse_x, mouse_y);
            if (hover) frame.fill(ad.x, ad.y, ad.w, ad.h, rgba(248, 242, 218));
            frame.fill(ad.x, ad.y + ad.h - 1, ad.w, 1, rule);
            if (hover) frame.fill(ad.x, ad.y, 3, ad.h, rgba(120, 49, 42));

            const CarSpec& car = cars[i];
            text(ad.x + 10, ad.y + 8, car_display_name(car), GlyphAtlas::Tone::Dark, 2);
            if (!car.variant.empty()) {
                text(ad.x + 10, ad.y + 32, car.variant, GlyphAtlas::Tone::Dark, 1);
            }
            text(ad.x + 10, ad.y + 51,
                 "$" + std::to_string(car.price) + "   " +
                 std::to_string(display_kw(car.horsepower)) + " kW   " +
                 std::to_string(display_kg(car.weight_lb)) + " kg",
                 GlyphAtlas::Tone::Dark, 1);

            const int copy_width = ad.w - 20;
            if (CarArt* art = art_for(car.id)) {
                const int preview_w = std::min(150, std::max(110, ad.w / 4));
                const int preview_h = std::max(1, art->source.height() * preview_w / art->source.width());
                SurfaceOwner preview = scaled_copy(art->source, preview_w, preview_h);
                frame.blit(preview, ad.x + ad.w - preview_w - 8,
                           ad.y + ad.h - preview_h - 7);
                (void)copy_width;
                text(ad.x + 10, ad.y + 72, blurbs[i % std::size(blurbs)],
                     GlyphAtlas::Tone::Dark, 1);
            } else {
                text(ad.x + 10, ad.y + 72, blurbs[i % std::size(blurbs)],
                     GlyphAtlas::Tone::Dark, 1);
            }
        }

        const Rect next_week{width - 385, height - 62, 160, 36};
        const Rect garage{width - 205, height - 62, 160, 36};
        text(46, height - 48,
             "BANKROLL $" + std::to_string(game.cash()) + "   " + message,
             GlyphAtlas::Tone::Dark, 1);
        newspaper_action(next_week, "NEXT WEEK");
        newspaper_action(garage, "GARAGE");
    }

    void draw_garage_au() {
        if (!garage_scaled.empty()) infiltratr_surface_copy(&frame.get(), &garage_scaled.get());
        else frame.clear(rgba(45, 42, 38));
        status("BACKYARD GARAGE");
        const bool moving = motion != Motion::Idle;
        if (displayed) {
            const OwnedCar& car = *displayed;
            panel({26, 82, 470, 138});
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
