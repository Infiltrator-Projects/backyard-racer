        (void)gap;
        const auto& cars = game.classifieds();
        for (std::size_t i = 0; i < cars.size(); ++i) {
            const Rect card = classified_ad_rect(i);
            if (!card.contains(x, y)) continue;
            selected_classified_index = i;
            message.clear();
            screen = Screen::ClassifiedArticle;
            dirty = true;
            return;
        }
        if (newspaper_next_week_rect().contains(x, y)) {
            selected_classified_index.reset();
            game.advance_week();
            message = "WEEK " + std::to_string(game.current_week()) + " - NEW CLASSIFIEDS";
            dirty = true;
            return;
        }
        if (newspaper_garage_rect().contains(x, y)) {
            selected_classified_index.reset();
            screen = Screen::Garage;
            sync_garage_to_active();
        }
    }

    void click_classified_article(int x, int y) {
        if (newspaper_article_back_rect().contains(x, y)) {
            selected_classified_index.reset();
            message.clear();
            screen = Screen::Classifieds;
            dirty = true;
            return;
        }

        if (!newspaper_article_buy_rect().contains(x, y)) return;
        if (!selected_classified_index ||
            *selected_classified_index >= game.classifieds().size()) {
            selected_classified_index.reset();
            message = "THAT AD IS NO LONGER AVAILABLE";
            screen = Screen::Classifieds;
            dirty = true;
            return;
        }

        const std::size_t listing_index = *selected_classified_index;
        const auto old_car = displayed;
        const auto old_index = displayed_index;
        std::string error;
        if (game.buy_car(listing_index, &error)) {
            const auto new_car = copy_active(game);
            const auto new_index = new_car
                ? std::optional<std::size_t>(game.active_car_index())
                : std::nullopt;
            message = "PURCHASED " + car_display_name(game.active_car()->base) +
                      (game.active_car()->base.variant.empty() ? std::string() :
                          " - " + game.active_car()->base.variant) +
                      " - " + game.active_car()->paint.name;
            selected_classified_index.reset();
            screen = Screen::Garage;
            begin_change(old_car, old_index, new_car, new_index);
        } else {
            message = error;
        }
        dirty = true;
    }

    void click_garage(int x, int y) {
        if (motion != Motion::Idle) return;
        if (Rect{38, 204, 138, 34}.contains(x, y) && game.active_car()) {
            int price = 0;
            std::string error;
            if (game.repair_active_car(&price, &error)) message = "REPAIRED FOR $" + std::to_string(price);
            else message = error;
            displayed = copy_active(game);
            dirty = true;
            return;
        }
        if (Rect{190, 204, 138, 34}.contains(x, y) && game.active_car()) {
            const auto old_car = displayed;
            const auto old_index = displayed_index;
            int price = 0;
            std::string error;
            if (game.sell_active_car(&price, &error)) {
                message = "SOLD FOR $" + std::to_string(price);
                const auto next = copy_active(game);
                const auto next_index = next ? std::optional<std::size_t>(game.active_car_index()) : std::nullopt;
                begin_change(old_car, old_index, next, next_index);
            } else message = error;
            dirty = true;
            return;
        }
        if (Rect{342, 204, 190, 34}.contains(x, y) && game.active_car()) {
            const bool install = !game.active_car()->front_bumper_installed;
            int price = 0;
            std::string error;
            if (game.set_bumper(BumperPosition::Front, install, &price, &error)) {
                message = std::string("FRONT BUMPER ") + (install ? "FITTED" : "REMOVED") +
                          " FOR $" + std::to_string(price);
                displayed = copy_active(game);
            } else {
                message = error;
            }
            dirty = true;
            return;
        }
        if (Rect{546, 204, 190, 34}.contains(x, y) && game.active_car()) {
            const bool install = !game.active_car()->rear_bumper_installed;
            int price = 0;
            std::string error;
            if (game.set_bumper(BumperPosition::Rear, install, &price, &error)) {
                message = std::string("REAR BUMPER ") + (install ? "FITTED" : "REMOVED") +
                          " FOR $" + std::to_string(price);
                displayed = copy_active(game);
            } else {
                message = error;
            }
            dirty = true;
            return;
        }
        if (Rect{30, height - 57, 174, 38}.contains(x, y)) screen = Screen::Classifieds;
        else if (Rect{216, height - 57, 154, 38}.contains(x, y) && game.active_car()) screen = Screen::Parts;
        else if (Rect{382, height - 57, 148, 38}.contains(x, y) && game.active_car()) screen = Screen::Diner;
        else if (Rect{542, height - 57, 150, 38}.contains(x, y) && game.garage().size() > 1) {
            const auto old_car = displayed;
            const auto old_index = displayed_index;
            game.next_car();
            const auto next = copy_active(game);
            begin_change(old_car, old_index, next,
                         next ? std::optional<std::size_t>(game.active_car_index()) : std::nullopt);
        } else if (Rect{width - 180, height - 57, 150, 38}.contains(x, y)) screen = Screen::Menu;
        dirty = true;
    }

    void click_parts(int x, int y) {
        if (Rect{30, height - 57, 150, 38}.contains(x, y)) {
            screen = Screen::Garage;
            sync_garage_to_active();
            dirty = true;
            return;
        }
        if (!game.active_car()) return;

        const int left_x = 54;
        const int right_x = width / 2 + 20;
        const int left_w = width / 2 - 82;
        const int right_w = width - right_x - 54;
        const int row_h = 34;
        const int first_row_y = 174;

        const auto& parts = game.parts_catalog();
        for (std::size_t i = 0; i < parts.size(); ++i) {
            const int row_y = first_row_y + static_cast<int>(i) * row_h;
            if (row_y + row_h > height - 132) break;
            if (Rect{left_x, row_y, left_w, row_h - 3}.contains(x, y)) {
                std::string error;
                if (game.buy_part(i, &error)) message = "INSTALLED " + parts[i].name;
                else message = error;
                dirty = true;
                return;
            }
        }

        const auto engines = game.compatible_engines();
        for (std::size_t i = 0; i < engines.size(); ++i) {
            const int row_y = first_row_y + static_cast<int>(i) * row_h;
            if (row_y + row_h > height - 132) break;
            if (Rect{right_x, row_y, right_w, row_h - 3}.contains(x, y)) {
                std::string error;
                int price = 0;
                const std::string name = engines[i].name;
                if (game.swap_engine(engines[i].id, &price, &error)) {
                    message = "INSTALLED " + name + " FOR $" + std::to_string(price);
                    displayed = copy_active(game);
                    vehicle_cache.id.clear();
                } else {
                    message = error;
                }
                dirty = true;
                return;
            }
        }

        if (!game.spare_parts().empty() && y >= height - 122 && y <= height - 96) {
            std::string error;
            const std::string name = game.spare_parts().front().name;
            if (game.install_spare(0, &error)) {
                message = "INSTALLED SPARE " + name;
                displayed = copy_active(game);
                vehicle_cache.id.clear();
            } else message = error;
            dirty = true;
        }
    }

    void click_diner(int x, int y) {
        if (Rect{30, height - 57, 150, 38}.contains(x, y)) {
            screen = Screen::Garage;
            sync_garage_to_active();
        } else if (Rect{124, 330, 210, 46}.contains(x, y)) start_race(100, false);
        else if (Rect{350, 330, 210, 46}.contains(x, y)) start_race(250, false);
        else if (Rect{124, 394, 436, 46}.contains(x, y)) start_race(0, true);
        dirty = true;
    }

    void on_button_press(PixelMouseButton button_value, int x, int y) {
        mouse_x = x;
        mouse_y = y;
        if (button_value != PixelMouseButton::Left) return;
        switch (screen) {
            case Screen::Menu:
                if (menu_new().contains(x, y)) {
                    game.new_game();
                    selected_classified_index.reset();
                    displayed.reset();
                    displayed_index.reset();
                    motion = Motion::Idle;
                    message = "NEW GAME - PICK YOUR FIRST CAR";
                    screen = Screen::Classifieds;
                } else if (menu_continue().contains(x, y) && game.started()) {
                    screen = game.active_car() ? Screen::Garage : Screen::Classifieds;
                    sync_garage_to_active();
                } else if (menu_settings().contains(x, y)) screen = Screen::Settings;
                else if (menu_quit().contains(x, y)) running = false;
                break;
            case Screen::Classifieds: click_classifieds(x, y); break;
            case Screen::ClassifiedArticle: click_classified_article(x, y); break;
            case Screen::Garage: click_garage(x, y); break;
            case Screen::Parts: click_parts(x, y); break;
            case Screen::Diner: click_diner(x, y); break;
            case Screen::Race:
                break;
            case Screen::Result:
                if (Rect{width / 2 - 260, 380, 230, 48}.contains(x, y)) {
                    screen = Screen::Garage;
                    sync_garage_to_active();
                } else if (Rect{width / 2 + 30, 380, 230, 48}.contains(x, y)) {
                    screen = Screen::Diner;
                }
                break;
            case Screen::Settings:
                if (Rect{width / 2 - 100, 370, 200, 46}.contains(x, y)) screen = Screen::Menu;
                break;
        }
        dirty = true;
    }

    void on_key_press(PixelKey key) {
        if (key == PixelKey::Escape) {
            if (screen == Screen::Menu) running = false;
            else if (screen == Screen::ClassifiedArticle) {
                selected_classified_index.reset();
                message.clear();
                screen = Screen::Classifieds;
            } else if (screen == Screen::Race) {
                race.set_throttle(false);
                screen = Screen::Diner;
            } else {
                screen = Screen::Menu;
            }
            dirty = true;
            return;
        }
        if (screen == Screen::Race) {
            if (key == PixelKey::Space) race.set_throttle(true);
            else if (key == PixelKey::Up || key == PixelKey::W) race.shift_up();
            else if (key == PixelKey::Down || key == PixelKey::S) race.shift_down();
            dirty = true;
        }
    }

    void on_key_release(PixelKey key) {
        if (screen == Screen::Race && key == PixelKey::Space) {
            race.set_throttle(false);
            dirty = true;
        }
    }

#include "pixel_menu_skin.inc"
};

PixelApp::PixelApp(int width, int height) : impl_(std::make_unique<Impl>(width, height)) {}
PixelApp::~PixelApp() = default;

bool PixelApp::running() const { return impl_->running; }
bool PixelApp::dirty() const { return impl_->dirty; }
void PixelApp::rendered() { impl_->dirty = false; }
int PixelApp::width() const { return impl_->width; }
int PixelApp::height() const { return impl_->height; }
const InfiltratrSurface& PixelApp::surface() const { return impl_->frame.get(); }

void PixelApp::resize(int width, int height) { impl_->resize(width, height); }
void PixelApp::tick(double dt_seconds) { impl_->tick(dt_seconds); }
void PixelApp::draw() { impl_->draw(); }
void PixelApp::on_motion(int x, int y) { impl_->on_motion(x, y); }
void PixelApp::on_button_press(PixelMouseButton button, int x, int y) { impl_->on_button_press(button, x, y); }
void PixelApp::on_button_release(PixelMouseButton, int, int) {}
void PixelApp::on_key_press(PixelKey key) { impl_->on_key_press(key); }
void PixelApp::on_key_release(PixelKey key) { impl_->on_key_release(key); }
void PixelApp::request_quit() { impl_->running = false; }

} // namespace backyard_racer