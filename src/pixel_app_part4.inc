            const int row = static_cast<int>(i / 2);
            const Rect card{left + col * (card_w + gap), top + row * (card_h + 10), card_w, card_h};
            if (card.contains(mouse_x, mouse_y)) frame.fill(card.x, card.y, card.w, card.h, rgba(248, 241, 214));
            outline(frame.get(), card.x, card.y, card.w, card.h, rgba(68, 61, 50),
                    card.contains(mouse_x, mouse_y) ? 3 : 1);
            text(card.x + 12, card.y + 11, car_display_name(cars[i]), GlyphAtlas::Tone::Dark, 2);
            if (!cars[i].variant.empty()) {
                text(card.x + 12, card.y + 38, cars[i].variant, GlyphAtlas::Tone::Dark, 1);
            }
            text(card.x + 12, card.y + 59,
                 "$" + std::to_string(cars[i].price) + "  " +
                 std::to_string(cars[i].horsepower) + " HP  " +
                 std::to_string(cars[i].weight_lb) + " LB",
                 GlyphAtlas::Tone::Dark, 2);

            if (CarArt* art = art_for(cars[i].id)) {
                const int preview_w = 165;
                const int preview_h = std::max(1, art->source.height() * preview_w / art->source.width());
                SurfaceOwner preview = scaled_copy(art->source, preview_w, preview_h);
                frame.blit(preview, card.x + card.w - preview_w - 10,
                           card.y + card.h - preview_h - 5);
            } else {
                text(card.x + 12, card.y + 86, "MODEL ART PENDING", GlyphAtlas::Tone::Dark, 1);
            }
        }
        text(width - 185, 42, "WEEK " + std::to_string(game.current_week()), GlyphAtlas::Tone::Dark, 2);
        button({width - 385, height - 62, 160, 36}, "NEXT WEEK");
        button({width - 205, height - 62, 160, 36}, "GARAGE");
        text(48, height - 42, message, GlyphAtlas::Tone::Dark, 2);
    }

    void draw_garage() {
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
                 "  HP " + std::to_string(car.horsepower()) +
                 "  WT " + std::to_string(car.base.weight_lb) + " LB",
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

    void draw_parts() {
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
                 "  " + std::to_string(car.base.horsepower) + " HP",
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
                     engines[i].name + "  " + std::to_string(engines[i].horsepower) +
                     " HP  $" + std::to_string(engines[i].price),
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

    void draw_diner() {
        frame.clear(rgba(26, 24, 31));
        frame.fill(0, height * 58 / 100, width, height * 42 / 100, rgba(39, 42, 48));
        status("MIDNIGHT DRIVE-IN");
        panel({90, 110, 560, 390});
        text(124, 142, "RACE BOARD", GlyphAtlas::Tone::Gold, 3);
        const Opponent preview = game.current_opponent();
        text(124, 198, "NEXT: " + preview.name, GlyphAtlas::Tone::Pale, 2);
        text(124, 228, car_display_name(preview.car.base), GlyphAtlas::Tone::Pale, 2);
        if (game.active_car()) {
            text(124, 270, "YOUR CAR: " + car_display_name(game.active_car()->base), GlyphAtlas::Tone::Pale, 2);
            button({124, 330, 210, 46}, "RACE $100", game.cash() >= 100);
            button({350, 330, 210, 46}, "RACE $250", game.cash() >= 250);
            button({124, 394, 436, 46}, "RACE FOR PINKS", true);
        } else {
            text(124, 310, "BUY A CAR BEFORE YOU RACE.", GlyphAtlas::Tone::Red, 2);
        }
        button({30, height - 57, 150, 38}, "GARAGE");
        text(210, height - 47, message, GlyphAtlas::Tone::Pale, 2);
    }

    void draw_race() {
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

        panel({24, 78, 560, 138});
        text(42, 94, race.green() ? "GREEN" : "STAGE", race.green() ? GlyphAtlas::Tone::Gold : GlyphAtlas::Tone::Red, 3);
        text(42, 132,
             "GEAR " + std::to_string(race.gear()) +
             "  RPM " + std::to_string(race.rpm()) +
             "  MPH " + std::to_string(static_cast<int>(std::lround(race.speed_mph()))),
             GlyphAtlas::Tone::Pale, 2);
        text(42, 164,
             "DIST " + std::to_string(static_cast<int>(std::lround(race.distance_ft()))) +
             " FT  SPACE THROTTLE  UP/DOWN SHIFT",
             GlyphAtlas::Tone::Pale, 2);
    }

    void draw_result() {
        frame.clear(rgba(32, 29, 26));
        status("RACE RESULT");
        panel({width / 2 - 360, 120, 720, 390});
        text_center({width / 2 - 330, 145, 660, 54}, result.summary,
                    result.won ? GlyphAtlas::Tone::Gold : GlyphAtlas::Tone::Red, 3);
        text(width / 2 - 270, 230,
             "YOUR ET " + fixed_ascii(result.player_et, 2) +
             "   THEIR ET " + fixed_ascii(result.opponent_et, 2),
             GlyphAtlas::Tone::Pale, 2);
        text(width / 2 - 270, 274,
             "CASH " + std::to_string(result.cash_delta) +
             "   REP " + std::to_string(result.reputation_delta) +
             "   WEAR " + std::to_string(result.wear),
             GlyphAtlas::Tone::Pale, 2);
        button({width / 2 - 260, 380, 230, 48}, "GARAGE");
        button({width / 2 + 30, 380, 230, 48}, "DRIVE-IN");
    }

    void draw_settings() {
        frame.clear(rgba(24, 24, 25));
        status("SETTINGS");
        panel({width / 2 - 340, 120, 680, 340});
        text_center({width / 2 - 300, 150, 600, 50}, "PIXEL FRAMEBUFFER PRESENTATION",
                    GlyphAtlas::Tone::Gold, 2);
        text(width / 2 - 270, 230, "X11 ONLY PRESENTS THE COMPLETED FRAME.", GlyphAtlas::Tone::Pale, 2);
        text(width / 2 - 270, 270, "GAME UI AND TEXT ARE PIXELS IN FRAME MEMORY.", GlyphAtlas::Tone::Pale, 2);
        button({width / 2 - 100, 370, 200, 46}, "BACK");
    }

    void draw() {
        switch (screen) {
            case Screen::Menu: draw_menu(); break;
            case Screen::Classifieds: draw_classifieds(); break;
            case Screen::Garage: draw_garage(); break;
            case Screen::Parts: draw_parts(); break;
            case Screen::Diner: draw_diner(); break;
            case Screen::Race: draw_race(); break;
            case Screen::Result: draw_result(); break;
            case Screen::Settings: draw_settings(); break;
        }
    }

    void start_race(int new_wager, bool pink) {
        const OwnedCar* car = game.active_car();
        if (!car) {
            message = "BUY A CAR FIRST";
            return;
        }
        if (!pink && game.cash() < new_wager) {
            message = "YOU CANNOT COVER THAT BET";
            return;
        }
        opponent = game.current_opponent();
        wager = new_wager;
        pink_slip = pink;
        race_settled = false;
        race.start(*car, opponent);
        screen = Screen::Race;
        dirty = true;
    }

    void tick(double dt) {
        if (tick_motion(dt)) dirty = true;
        if (screen == Screen::Race && race.active()) {
            race.update(dt);
            dirty = true;
            if (race.finished() && !race_settled) {
                result = game.settle_race(opponent, wager, pink_slip, race.player_et());
                race_settled = true;
                race.set_throttle(false);
                screen = Screen::Result;
                dirty = true;
            }
        }
    }

    void on_motion(int x, int y) {
        if (x != mouse_x || y != mouse_y) {
            mouse_x = x;
            mouse_y = y;
            dirty = true;
        }
    }

    void click_classifieds(int x, int y) {
        const int gap = 14;
