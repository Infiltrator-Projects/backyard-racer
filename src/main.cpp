// SPDX-License-Identifier: GPL-3.0-or-later
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <infiltratr/core.h>
#include "gameplay.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace backyard_racer {

const InfiltratrProjectInfo& project_info() {
    static const InfiltratrProjectInfo info = {
        sizeof(InfiltratrProjectInfo), INFILTRATR_PROJECT_INFO_ABI,
        "Backyard Racer", "backyard-racer", "au.com.infiltrator.backyard-racer",
        "0.3.0-dev", "Infiltrator-Projects/backyard-racer", "development",
        "Shannon Smith", "https://github.com/Infiltrator-Projects/backyard-racer",
        "GPL-3.0-or-later", "Street-rod garage and racing game", "backyard-racer",
        "Copyright (c) 2026 Shannon Smith"
    };
    return info;
}

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
};

enum class Screen { Menu, Classifieds, Garage, Parts, Diner, Result, Settings };

class App {
public:
    App(int width, int height) : width_(width), height_(height) {
        dpy_ = XOpenDisplay(nullptr);
        if (!dpy_) throw std::runtime_error("Unable to open X11 display");
        screen_no_ = DefaultScreen(dpy_);
        visual_ = DefaultVisual(dpy_, screen_no_);
        win_ = XCreateSimpleWindow(dpy_, RootWindow(dpy_, screen_no_), 70, 70,
                                   static_cast<unsigned>(width_), static_cast<unsigned>(height_), 0,
                                   BlackPixel(dpy_, screen_no_), BlackPixel(dpy_, screen_no_));
        XStoreName(dpy_, win_, project_info().program_name);
        XSelectInput(dpy_, win_, ExposureMask | KeyPressMask | ButtonPressMask |
                                 PointerMotionMask | StructureNotifyMask);
        wm_delete_ = XInternAtom(dpy_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dpy_, win_, &wm_delete_, 1);
        gc_ = XCreateGC(dpy_, win_, 0, nullptr);
        font_ = XLoadQueryFont(dpy_, "9x15bold");
        if (!font_) font_ = XLoadQueryFont(dpy_, "fixed");
        if (font_) XSetFont(dpy_, gc_, font_->fid);
        XMapWindow(dpy_, win_);
    }

    ~App() {
        if (font_) XFreeFont(dpy_, font_);
        if (gc_) XFreeGC(dpy_, gc_);
        if (win_) XDestroyWindow(dpy_, win_);
        if (dpy_) XCloseDisplay(dpy_);
    }

    int run() {
        draw();
        while (running_) {
            XEvent ev{};
            XNextEvent(dpy_, &ev);
            switch (ev.type) {
                case Expose: draw(); break;
                case ConfigureNotify:
                    width_ = std::max(900, ev.xconfigure.width);
                    height_ = std::max(600, ev.xconfigure.height);
                    draw();
                    break;
                case MotionNotify:
                    mouse_x_ = ev.xmotion.x;
                    mouse_y_ = ev.xmotion.y;
                    draw();
                    break;
                case ButtonPress:
                    if (ev.xbutton.button == Button1) click(ev.xbutton.x, ev.xbutton.y);
                    draw();
                    break;
                case KeyPress:
                    key(XLookupKeysym(&ev.xkey, 0));
                    draw();
                    break;
                case ClientMessage:
                    if (static_cast<Atom>(ev.xclient.data.l[0]) == wm_delete_) running_ = false;
                    break;
                default: break;
            }
        }
        return 0;
    }

private:
    Display* dpy_ = nullptr;
    Visual* visual_ = nullptr;
    int screen_no_ = 0;
    Window win_ = 0;
    GC gc_ = 0;
    XFontStruct* font_ = nullptr;
    Atom wm_delete_ = 0;
    int width_ = 1280, height_ = 720;
    int mouse_x_ = 0, mouse_y_ = 0;
    bool running_ = true;
    Screen screen_ = Screen::Menu;
    GameState game_;
    RaceResult last_race_;
    std::string message_ = "PRE-ALPHA - INVESTOR VERTICAL SLICE";

    unsigned long rgb(unsigned r, unsigned g, unsigned b) const {
        auto channel = [](unsigned value, unsigned long mask) {
            if (!mask) return 0UL;
            unsigned shift = 0;
            while (((mask >> shift) & 1UL) == 0UL) ++shift;
            const unsigned long range = mask >> shift;
            return (((static_cast<unsigned long>(value) * range + 127UL) / 255UL) << shift) & mask;
        };
        return channel(r, visual_->red_mask) | channel(g, visual_->green_mask) | channel(b, visual_->blue_mask);
    }

    void color(unsigned r, unsigned g, unsigned b) { XSetForeground(dpy_, gc_, rgb(r, g, b)); }
    void fill(Rect r, unsigned cr, unsigned cg, unsigned cb) {
        color(cr, cg, cb);
        XFillRectangle(dpy_, win_, gc_, r.x, r.y,
                       static_cast<unsigned>(std::max(0, r.w)), static_cast<unsigned>(std::max(0, r.h)));
    }
    void outline(Rect r, unsigned cr, unsigned cg, unsigned cb, int thick = 1) {
        color(cr, cg, cb);
        for (int i = 0; i < thick; ++i)
            XDrawRectangle(dpy_, win_, gc_, r.x + i, r.y + i,
                           static_cast<unsigned>(std::max(0, r.w - 1 - 2 * i)),
                           static_cast<unsigned>(std::max(0, r.h - 1 - 2 * i)));
    }
    void text(int x, int y, const std::string& s, unsigned r = 235, unsigned g = 236, unsigned b = 228) {
        color(r, g, b);
        XDrawString(dpy_, win_, gc_, x, y, s.c_str(), static_cast<int>(s.size()));
    }
    void big_text(int x, int y, const std::string& s, unsigned r = 210, unsigned g = 215, unsigned b = 220) {
        text(x, y, s, r, g, b);
        text(x + 1, y, s, r, g, b);
        text(x, y + 1, s, r, g, b);
    }
    Rect button(int x, int y, int w, int h, const std::string& label, bool enabled = true) {
        Rect r{x, y, w, h};
        const bool hover = enabled && r.contains(mouse_x_, mouse_y_);
        fill(r, hover ? 55 : 24, hover ? 58 : 26, hover ? 62 : 29);
        outline(r, enabled ? 198 : 86, enabled ? 205 : 88, enabled ? 211 : 90, 2);
        text(x + 16, y + h / 2 + 5, label, enabled ? 235 : 110, enabled ? 236 : 112, enabled ? 228 : 114);
        return r;
    }

    void header(const std::string& title) {
        fill({0, 0, width_, 78}, 17, 18, 20);
        big_text(26, 34, "BACKYARD RACER", 205, 211, 216);
        text(26, 60, title, 190, 192, 194);
        text(width_ - 230, 35, "CASH " + std::to_string(game_.cash()), 220, 220, 205);
    }

    void draw_car(int cx, int base_y, int w, unsigned r, unsigned g, unsigned b) {
        const int h = std::max(60, w / 5), x = cx - w / 2, y = base_y - h;
        fill({x + w / 12, y + h / 3, w * 10 / 12, h * 2 / 3}, r, g, b);
        XPoint roof[4] = {
            {static_cast<short>(x + w / 4), static_cast<short>(y + h / 3)},
            {static_cast<short>(x + w * 2 / 5), static_cast<short>(y)},
            {static_cast<short>(x + w * 3 / 4), static_cast<short>(y)},
            {static_cast<short>(x + w * 10 / 12), static_cast<short>(y + h / 3)}
        };
        color(r, g, b);
        XFillPolygon(dpy_, win_, gc_, roof, 4, Convex, CoordModeOrigin);
        fill({x + w * 43 / 100, y + 8, w * 25 / 100, h / 4}, 36, 48, 59);
        for (int wx : {x + w / 5, x + w * 4 / 5 - 34}) {
            fill({wx, base_y - 30, 34, 34}, 12, 12, 12);
            fill({wx + 9, base_y - 21, 16, 16}, 160, 164, 168);
        }
    }

    void draw_menu() {
        fill({0, 0, width_, height_}, 22, 27, 35);
        fill({0, height_ * 2 / 3, width_, height_ / 3}, 48, 47, 45);
        fill({width_ * 57 / 100, 150, width_ * 35 / 100, height_ * 50 / 100}, 71, 68, 64);
        fill({width_ * 61 / 100, 210, width_ * 27 / 100, height_ * 42 / 100}, 24, 24, 24);
        draw_car(width_ * 75 / 100, height_ * 72 / 100, width_ * 34 / 100, 145, 34, 32);
        fill({45, 45, 520, 590}, 7, 8, 9);
        big_text(72, 105, "BACKYARD RACER", 215, 220, 224);
        text(74, 145, "BUILD IT  RACE IT  RISK IT", 232, 232, 220);
        button(72, 250, 330, 48, "NEW GAME");
        button(72, 312, 330, 48, "CONTINUE", game_.started());
        button(72, 374, 330, 48, "SETTINGS");
        button(72, 436, 330, 48, "QUIT");
        text(72, 575, message_, 180, 183, 185);
    }

    void draw_classifieds() {
        fill({0, 0, width_, height_}, 221, 214, 188);
        header("THE DAILY GAZETTE - USED CARS");
        text(40, 112, "USED CARS - CLICK A LISTING TO BUY", 28, 28, 28);
        const auto& cars = game_.classifieds();
        const int gap = 18, left = 40, top = 140, card_w = (width_ - 100) / 2, card_h = 105;
        for (std::size_t i = 0; i < cars.size(); ++i) {
            const int col = static_cast<int>(i % 2), row = static_cast<int>(i / 2);
            Rect card{left + col * (card_w + gap), top + row * (card_h + 12), card_w, card_h};
            const bool hover = card.contains(mouse_x_, mouse_y_);
            fill(card, hover ? 246 : 238, hover ? 240 : 232, hover ? 214 : 206);
            outline(card, 35, 35, 35, 2);
            text(card.x + 14, card.y + 27, car_display_name(cars[i]), 25, 25, 25);
            text(card.x + 14, card.y + 54,
                 "PRICE " + std::to_string(cars[i].price) + "   HP " + std::to_string(cars[i].horsepower), 50, 50, 50);
            text(card.x + 14, card.y + 80, "WEIGHT " + std::to_string(cars[i].weight_lb) + " LB", 70, 70, 70);
        }
        button(width_ - 210, height_ - 58, 170, 38, "GARAGE");
        text(40, height_ - 34, message_, 60, 50, 40);
    }

    void draw_garage() {
        fill({0, 0, width_, height_}, 103, 99, 91);
        header("BACKYARD GARAGE");
        const OwnedCar* car = game_.active_car();
        fill({0, height_ * 67 / 100, width_, height_ * 33 / 100}, 59, 58, 55);

        if (car) {
            draw_car(width_ / 2, height_ * 67 / 100 - 15, std::min(620, width_ / 2), 151, 39, 36);
            big_text(45, 135, car_display_name(car->base), 235, 236, 228);
            text(45, 165, "HP " + std::to_string(car->horsepower()) + "   WEIGHT " +
                 std::to_string(car->base.weight_lb) + " LB", 225, 225, 214);
            std::ostringstream tr;
            tr << std::fixed << std::setprecision(2) << car->traction();
            text(45, 190, "TRACTION " + tr.str() + "   INSTALLED " +
                 std::to_string(car->installed_parts.size()) + "   SPARES " +
                 std::to_string(game_.spare_parts().size()), 225, 225, 214);

            const int list_x = width_ - 330;
            text(list_x, 120, "INSTALLED PARTS", 235, 236, 228);
            int y = 148;
            if (car->installed_parts.empty()) {
                text(list_x, y, "STOCK", 205, 205, 195);
            } else {
                for (const auto& part : car->installed_parts) {
                    text(list_x, y, part_type_name(part.type) + ": " + part.name, 205, 205, 195);
                    y += 24;
                }
            }
        } else {
            big_text(45, 150, "EMPTY GARAGE", 235, 236, 228);
            text(45, 182, "BUY YOUR FIRST CAR FROM THE CLASSIFIEDS", 230, 230, 218);
        }

        button(35, height_ - 66, 180, 42, "CLASSIFIEDS");
        button(230, height_ - 66, 170, 42, "PARTS BIN", car != nullptr);
        button(415, height_ - 66, 150, 42, "DINER", car != nullptr);
        button(580, height_ - 66, 160, 42, "NEXT CAR", game_.garage().size() > 1);
        button(width_ - 200, height_ - 66, 165, 42, "MAIN MENU");
        text(35, height_ - 92, message_, 225, 225, 214);
    }

    Rect catalog_rect(std::size_t i) const {
        const int split = width_ * 63 / 100;
        const int left = 30, top = 140, gap = 12;
        const int card_w = (split - 75) / 2, card_h = 70;
        const int col = static_cast<int>(i % 2), row = static_cast<int>(i / 2);
        return {left + col * (card_w + gap), top + row * (card_h + gap), card_w, card_h};
    }

    Rect spare_rect(std::size_t i) const {
        const int split = width_ * 63 / 100;
        const int x = split + 24, y = 140 + static_cast<int>(i) * 62;
        return {x, y, width_ - x - 30, 50};
    }

    void draw_parts() {
        fill({0, 0, width_, height_}, 63, 64, 66);
        header("GARAGE PARTS - BUY, SWAP AND KEEP YOUR OLD HARDWARE");
        const int split = width_ * 63 / 100;
        text(30, 112, "NEW PARTS - CLICK TO BUY + INSTALL", 220, 220, 210);
        text(split + 24, 112, "YOUR PARTS BIN - CLICK TO REINSTALL", 220, 220, 210);

        const auto& parts = game_.parts_catalog();
        for (std::size_t i = 0; i < parts.size(); ++i) {
            const Rect r = catalog_rect(i);
            const bool hover = r.contains(mouse_x_, mouse_y_);
            fill(r, hover ? 45 : 29, hover ? 48 : 31, hover ? 52 : 34);
            outline(r, 150, 155, 160, 1);
            text(r.x + 12, r.y + 22, parts[i].name, 235, 236, 228);
            text(r.x + 12, r.y + 46,
                 part_type_name(parts[i].type) + "   $" + std::to_string(parts[i].price) +
                 "   +" + std::to_string(parts[i].horsepower_gain) + " HP", 190, 194, 196);
        }

        const auto& spares = game_.spare_parts();
        if (spares.empty()) {
            text(split + 24, 160, "EMPTY - REPLACED PARTS WILL APPEAR HERE", 170, 173, 175);
        } else {
            for (std::size_t i = 0; i < spares.size() && i < 7; ++i) {
                const Rect r = spare_rect(i);
                const bool hover = r.contains(mouse_x_, mouse_y_);
                fill(r, hover ? 72 : 45, hover ? 68 : 43, hover ? 58 : 39);
                outline(r, 176, 164, 136, 1);
                text(r.x + 10, r.y + 20, spares[i].name, 235, 230, 210);
                text(r.x + 10, r.y + 40, part_type_name(spares[i].type) + " - INSTALL FREE", 190, 185, 170);
            }
        }

        button(width_ - 200, height_ - 58, 165, 38, "GARAGE");
        text(30, height_ - 34, message_, 220, 220, 210);
    }

    void draw_diner() {
        fill({0, 0, width_, height_}, 34, 29, 27);
        header("THE DINER - FIND A RACE");
        const Opponent opp = game_.current_opponent();
        fill({65, 130, width_ - 130, 300}, 19, 20, 21);
        outline({65, 130, width_ - 130, 300}, 165, 168, 170, 2);
        big_text(100, 180, opp.name + " WANTS TO RACE", 232, 232, 220);
        text(100, 220, car_display_name(opp.car.base), 210, 210, 200);
        text(100, 250, "HP " + std::to_string(opp.car.horsepower()) + "   WEIGHT " +
             std::to_string(opp.car.base.weight_lb) + " LB", 210, 210, 200);
        text(100, 292, "CHOOSE THE STAKES", 190, 192, 194);
        button(100, 325, 180, 48, "RACE FOR $100");
        button(300, 325, 180, 48, "RACE FOR $250");
        button(500, 325, 210, 48, "RACE FOR PINKS");
        button(width_ - 200, height_ - 58, 165, 38, "GARAGE");
        text(70, height_ - 34, message_, 200, 200, 190);
    }

    void draw_result() {
        fill({0, 0, width_, height_}, 23, 24, 25);
        header("RACE RESULT");
        const bool won = last_race_.won;
        big_text(90, 165, last_race_.summary, won ? 120 : 220, won ? 210 : 95, won ? 120 : 80);
        if (last_race_.valid) {
            std::ostringstream p, o;
            p << std::fixed << std::setprecision(2) << last_race_.player_et;
            o << std::fixed << std::setprecision(2) << last_race_.opponent_et;
            text(90, 220, "YOUR ET      " + p.str() + " SEC", 225, 225, 214);
            text(90, 250, "OPPONENT ET  " + o.str() + " SEC", 225, 225, 214);
            if (!last_race_.pink_slip)
                text(90, 290, "CASH CHANGE  " + std::to_string(last_race_.cash_delta), 225, 225, 214);
        }
        button(90, 350, 220, 50, "BACK TO GARAGE");
    }

    void draw_settings() {
        fill({0, 0, width_, height_}, 25, 26, 28);
        header("SETTINGS");
        big_text(70, 155, "SETTINGS ARE NEXT", 220, 220, 210);
        text(70, 195, "THE INVESTOR BUILD IS FOCUSED ON THE COMPLETE GAMEPLAY LOOP.", 190, 192, 194);
        button(70, 250, 190, 44, "MAIN MENU");
    }

    void draw() {
        XClearWindow(dpy_, win_);
        switch (screen_) {
            case Screen::Menu: draw_menu(); break;
            case Screen::Classifieds: draw_classifieds(); break;
            case Screen::Garage: draw_garage(); break;
            case Screen::Parts: draw_parts(); break;
            case Screen::Diner: draw_diner(); break;
            case Screen::Result: draw_result(); break;
            case Screen::Settings: draw_settings(); break;
        }
        XFlush(dpy_);
    }

    void key(KeySym key_sym) {
        if (key_sym != XK_Escape) return;
        if (screen_ == Screen::Menu) running_ = false;
        else if (screen_ == Screen::Garage) screen_ = Screen::Menu;
        else if (screen_ == Screen::Result || screen_ == Screen::Parts ||
                 screen_ == Screen::Diner || screen_ == Screen::Classifieds)
            screen_ = Screen::Garage;
        else
            screen_ = Screen::Menu;
    }

    void click(int x, int y) {
        if (screen_ == Screen::Menu) {
            if (Rect{72, 250, 330, 48}.contains(x, y)) {
                game_.new_game();
                message_ = "PICK YOUR FIRST CAR";
                screen_ = Screen::Classifieds;
            } else if (Rect{72, 312, 330, 48}.contains(x, y) && game_.started()) {
                screen_ = game_.garage().empty() ? Screen::Classifieds : Screen::Garage;
            } else if (Rect{72, 374, 330, 48}.contains(x, y)) {
                screen_ = Screen::Settings;
            } else if (Rect{72, 436, 330, 48}.contains(x, y)) {
                running_ = false;
            }
            return;
        }

        if (screen_ == Screen::Classifieds) {
            const int card_w = (width_ - 100) / 2, card_h = 105, gap = 18, left = 40, top = 140;
            const auto& cars = game_.classifieds();
            for (std::size_t i = 0; i < cars.size(); ++i) {
                const int col = static_cast<int>(i % 2), row = static_cast<int>(i / 2);
                Rect r{left + col * (card_w + gap), top + row * (card_h + 12), card_w, card_h};
                if (r.contains(x, y)) {
                    std::string error;
                    if (game_.buy_car(i, &error)) {
                        message_ = "CAR BOUGHT - IT IS IN YOUR GARAGE";
                        screen_ = Screen::Garage;
                    } else {
                        message_ = error;
                    }
                    return;
                }
            }
            if (Rect{width_ - 210, height_ - 58, 170, 38}.contains(x, y)) screen_ = Screen::Garage;
            return;
        }

        if (screen_ == Screen::Garage) {
            if (Rect{35, height_ - 66, 180, 42}.contains(x, y)) screen_ = Screen::Classifieds;
            else if (Rect{230, height_ - 66, 170, 42}.contains(x, y) && game_.active_car()) screen_ = Screen::Parts;
            else if (Rect{415, height_ - 66, 150, 42}.contains(x, y) && game_.active_car()) screen_ = Screen::Diner;
            else if (Rect{580, height_ - 66, 160, 42}.contains(x, y) && game_.garage().size() > 1) {
                game_.next_car();
                message_ = "ACTIVE CAR CHANGED";
            } else if (Rect{width_ - 200, height_ - 66, 165, 42}.contains(x, y)) {
                screen_ = Screen::Menu;
            }
            return;
        }

        if (screen_ == Screen::Parts) {
            const auto& parts = game_.parts_catalog();
            for (std::size_t i = 0; i < parts.size(); ++i) {
                if (catalog_rect(i).contains(x, y)) {
                    std::string error;
                    if (game_.buy_part(i, &error))
                        message_ = parts[i].name + " INSTALLED - OLD PART SAVED IF REPLACED";
                    else
                        message_ = error;
                    return;
                }
            }

            const auto spare_count = game_.spare_parts().size();
            for (std::size_t i = 0; i < spare_count && i < 7; ++i) {
                if (spare_rect(i).contains(x, y)) {
                    const std::string name = game_.spare_parts()[i].name;
                    std::string error;
                    if (game_.install_spare(i, &error))
                        message_ = name + " REINSTALLED - SWAPPED PART RETURNED TO BIN";
                    else
                        message_ = error;
                    return;
                }
            }

            if (Rect{width_ - 200, height_ - 58, 165, 38}.contains(x, y)) screen_ = Screen::Garage;
            return;
        }

        if (screen_ == Screen::Diner) {
            if (Rect{100, 325, 180, 48}.contains(x, y)) {
                last_race_ = game_.race_for_cash(100);
                screen_ = Screen::Result;
            } else if (Rect{300, 325, 180, 48}.contains(x, y)) {
                last_race_ = game_.race_for_cash(250);
                screen_ = Screen::Result;
            } else if (Rect{500, 325, 210, 48}.contains(x, y)) {
                last_race_ = game_.race_for_pink_slip();
                screen_ = Screen::Result;
            } else if (Rect{width_ - 200, height_ - 58, 165, 38}.contains(x, y)) {
                screen_ = Screen::Garage;
            }
            return;
        }

        if (screen_ == Screen::Result) {
            if (Rect{90, 350, 220, 50}.contains(x, y)) {
                message_ = last_race_.summary;
                screen_ = game_.garage().empty() ? Screen::Classifieds : Screen::Garage;
            }
            return;
        }

        if (screen_ == Screen::Settings && Rect{70, 250, 190, 44}.contains(x, y)) screen_ = Screen::Menu;
    }
};

} // namespace backyard_racer

int main(int argc, char** argv) {
    const InfiltratrProjectInfo& info = backyard_racer::project_info();
    if (!infiltratr_project_info_is_valid(&info)) {
        std::cerr << "Backyard Racer project metadata is invalid\n";
        return 1;
    }
    if (argc == 2 && infiltratr_string_equal(argv[1], "--version")) {
        std::cout << info.program_name << ' ' << info.version << '\n';
        return 0;
    }
    if (argc == 2 && infiltratr_string_equal(argv[1], "--project-info"))
        return infiltratr_project_info_print(stdout, &info) == 0 ? 0 : 1;

    try {
        backyard_racer::App app(1280, 720);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << info.program_name << " failed to start: " << e.what() << '\n';
        return 1;
    }
}
