// SPDX-License-Identifier: GPL-3.0-or-later
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <infiltratr/core.h>
#include <infiltratr/timing.h>

#include "gameplay.h"
#include "race_session.h"
#include "generated_assets.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace backyard_racer {

const InfiltratrProjectInfo& project_info() {
    static const InfiltratrProjectInfo info = {
        sizeof(InfiltratrProjectInfo), INFILTRATR_PROJECT_INFO_ABI,
        "Backyard Racer", "backyard-racer", "au.com.infiltrator.backyard-racer",
        "0.6.0-dev", "Infiltrator-Projects/backyard-racer", "development",
        "Shannon Smith", "https://github.com/Infiltrator-Projects/backyard-racer",
        "GPL-3.0-or-later", "Street-rod garage and racing game", "backyard-racer",
        "Copyright (c) 2000-2026 Shannon Smith"
    };
    return info;
}

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    bool contains(int px, int py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};

enum class Screen { Menu, Classifieds, Garage, Parts, Diner, Race, Result, Settings };

class App {
public:
    App(int width, int height) : width_(width), height_(height) {
        dpy_ = XOpenDisplay(nullptr);
        if (!dpy_) throw std::runtime_error("Unable to open X11 display");
        screen_no_ = DefaultScreen(dpy_);
        visual_ = DefaultVisual(dpy_, screen_no_);
        win_ = XCreateSimpleWindow(dpy_, RootWindow(dpy_, screen_no_), 70, 70,
                                   static_cast<unsigned>(width_), static_cast<unsigned>(height_),
                                   0, BlackPixel(dpy_, screen_no_), BlackPixel(dpy_, screen_no_));
        XStoreName(dpy_, win_, project_info().program_name);
        XSelectInput(dpy_, win_, ExposureMask | KeyPressMask | KeyReleaseMask |
                                  ButtonPressMask | PointerMotionMask | StructureNotifyMask);
        wm_delete_ = XInternAtom(dpy_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dpy_, win_, &wm_delete_, 1);
        gc_ = XCreateGC(dpy_, win_, 0, nullptr);
        font_ = XLoadQueryFont(dpy_, "9x15bold");
        if (!font_) font_ = XLoadQueryFont(dpy_, "fixed");
        if (font_) XSetFont(dpy_, gc_, font_->fid);
        ensure_back_buffer();
        XMapWindow(dpy_, win_);
    }

    ~App() {
        if (back_buffer_) XFreePixmap(dpy_, back_buffer_);
        if (font_) XFreeFont(dpy_, font_);
        if (gc_) XFreeGC(dpy_, gc_);
        if (win_) XDestroyWindow(dpy_, win_);
        if (dpy_) XCloseDisplay(dpy_);
    }

    int run() {
        draw();
        while (running_) {
            if (screen_ == Screen::Race) {
                bool redraw = false;
                while (XPending(dpy_) > 0) {
                    XEvent ev{};
                    XNextEvent(dpy_, &ev);
                    process_event(ev);
                    redraw = true;
                }
                redraw = tick_race() || redraw;
                if (redraw) draw();
                if (screen_ == Screen::Race) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } else {
                XEvent ev{};
                XNextEvent(dpy_, &ev);
                process_event(ev);
                draw();
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
    Pixmap back_buffer_ = 0;
    unsigned int back_width_ = 0, back_height_ = 0;
    int width_ = 1280, height_ = 720;
    int mouse_x_ = 0, mouse_y_ = 0;
    bool running_ = true;
    Screen screen_ = Screen::Menu;

    GameState game_;
    RaceResult last_race_;
    DragRaceSession race_;
    Opponent race_opponent_;
    int race_wager_ = 0;
    bool race_pinks_ = false;
    InfiltratrFixedStepScheduler race_clock_{};
    bool race_clock_ready_ = false;
    std::string message_ = "SIDE-VIEW SCENE PASS - BUILD IT, RACE IT, RISK IT";

    static std::uint64_t monotonic_ns() {
        const auto n = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        return n > 0 ? static_cast<std::uint64_t>(n) : 0U;
    }

    Drawable canvas() const { return back_buffer_ ? static_cast<Drawable>(back_buffer_) : static_cast<Drawable>(win_); }

    void ensure_back_buffer() {
        const unsigned w = static_cast<unsigned>(std::max(1, width_));
        const unsigned h = static_cast<unsigned>(std::max(1, height_));
        if (back_buffer_ && w == back_width_ && h == back_height_) return;
        if (back_buffer_) XFreePixmap(dpy_, back_buffer_);
        back_buffer_ = XCreatePixmap(dpy_, win_, w, h, static_cast<unsigned>(DefaultDepth(dpy_, screen_no_)));
        if (!back_buffer_) throw std::runtime_error("Unable to allocate X11 back buffer");
        back_width_ = w; back_height_ = h;
    }

    void process_event(const XEvent& ev) {
        switch (ev.type) {
            case ConfigureNotify:
                width_ = std::max(900, ev.xconfigure.width);
                height_ = std::max(600, ev.xconfigure.height);
                ensure_back_buffer();
                break;
            case MotionNotify: mouse_x_ = ev.xmotion.x; mouse_y_ = ev.xmotion.y; break;
            case ButtonPress: if (ev.xbutton.button == Button1) click(ev.xbutton.x, ev.xbutton.y); break;
            case KeyPress: key_press(XLookupKeysym(const_cast<XKeyEvent*>(&ev.xkey), 0)); break;
            case KeyRelease: key_release(XLookupKeysym(const_cast<XKeyEvent*>(&ev.xkey), 0)); break;
            case ClientMessage: if (static_cast<Atom>(ev.xclient.data.l[0]) == wm_delete_) running_ = false; break;
            default: break;
        }
    }

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
        XFillRectangle(dpy_, canvas(), gc_, r.x, r.y, static_cast<unsigned>(std::max(0, r.w)), static_cast<unsigned>(std::max(0, r.h)));
    }
    void outline(Rect r, unsigned cr, unsigned cg, unsigned cb, int thick = 1) {
        color(cr, cg, cb);
        for (int i = 0; i < thick; ++i)
            XDrawRectangle(dpy_, canvas(), gc_, r.x + i, r.y + i,
                           static_cast<unsigned>(std::max(0, r.w - 1 - 2 * i)),
                           static_cast<unsigned>(std::max(0, r.h - 1 - 2 * i)));
    }
    void line(int x1, int y1, int x2, int y2, unsigned r, unsigned g, unsigned b, int thick = 1) {
        color(r, g, b); XSetLineAttributes(dpy_, gc_, thick, LineSolid, CapButt, JoinMiter);
        XDrawLine(dpy_, canvas(), gc_, x1, y1, x2, y2);
        XSetLineAttributes(dpy_, gc_, 1, LineSolid, CapButt, JoinMiter);
    }
    void text(int x, int y, const std::string& s, unsigned r = 235, unsigned g = 236, unsigned b = 228) {
        color(r, g, b); XDrawString(dpy_, canvas(), gc_, x, y, s.c_str(), static_cast<int>(s.size()));
    }
    void big_text(int x, int y, const std::string& s, unsigned r = 235, unsigned g = 236, unsigned b = 228) {
        text(x, y, s, r, g, b); text(x + 1, y, s, r, g, b); text(x, y + 1, s, r, g, b);
    }

    Rect button(int x, int y, int w, int h, const std::string& label, bool enabled = true) {
        Rect r{x, y, w, h}; const bool hover = enabled && r.contains(mouse_x_, mouse_y_);
        fill(r, hover ? 126U : 45U, hover ? 30U : 36U, hover ? 31U : 34U);
        outline(r, enabled ? 224U : 90U, enabled ? 207U : 90U, enabled ? 158U : 90U, 2);
        text(x + 13, y + h / 2 + 5, label, enabled ? 245U : 115U, enabled ? 238U : 115U, enabled ? 214U : 115U);
        return r;
    }

    void status_bar(const std::string& location) {
        fill({0, 0, width_, 64}, 17, 15, 14);
        fill({0, 60, width_, 4}, 150, 35, 28);
        big_text(22, 28, "BACKYARD RACER", 244, 222, 183);
        text(22, 51, location, 207, 191, 167);
        text(width_ - 430, 34,
             "CASH $" + std::to_string(game_.cash()) + "   REP " + std::to_string(game_.reputation()) +
             "   W " + std::to_string(game_.wins()) + " L " + std::to_string(game_.losses()), 235, 225, 199);
    }

    void brick_wall(int y0, int y1) {
        fill({0, y0, width_, y1 - y0}, 72, 61, 53);
        for (int y = y0; y < y1; y += 28) {
            line(0, y, width_, y, 47, 39, 35);
            const int offset = ((y - y0) / 28 % 2) ? 50 : 0;
            for (int x = offset; x < width_; x += 100) line(x, y, x, std::min(y + 28, y1), 47, 39, 35);
        }
    }

    void wheel(int cx, int cy, int radius) {
        color(18, 18, 18); XFillArc(dpy_, canvas(), gc_, cx - radius, cy - radius, radius * 2, radius * 2, 0, 360 * 64);
        color(192, 194, 190); XFillArc(dpy_, canvas(), gc_, cx - radius * 3 / 5, cy - radius * 3 / 5, radius * 6 / 5, radius * 6 / 5, 0, 360 * 64);
        color(43, 44, 45); XFillArc(dpy_, canvas(), gc_, cx - radius / 4, cy - radius / 4, radius / 2, radius / 2, 0, 360 * 64);
    }

    void draw_side_car(int cx, int base_y, int w, unsigned br, unsigned bg, unsigned bb) {
        const int h = std::max(90, w * 29 / 100);
        const int x = cx - w / 2;
        const int wheel_r = std::max(18, w / 14);
        const int wheel_y = base_y - wheel_r;
        const int rear_x = x + w * 23 / 100;
        const int front_x = x + w * 77 / 100;

        color(br, bg, bb);
        XPoint body[] = {{static_cast<short>(x + w * 4 / 100), static_cast<short>(base_y - h * 44 / 100)},
                         {static_cast<short>(x + w * 14 / 100), static_cast<short>(base_y - h * 61 / 100)},
                         {static_cast<short>(x + w * 34 / 100), static_cast<short>(base_y - h * 67 / 100)},
                         {static_cast<short>(x + w * 43 / 100), static_cast<short>(base_y - h * 94 / 100)},
                         {static_cast<short>(x + w * 64 / 100), static_cast<short>(base_y - h * 94 / 100)},
                         {static_cast<short>(x + w * 74 / 100), static_cast<short>(base_y - h * 68 / 100)},
                         {static_cast<short>(x + w * 94 / 100), static_cast<short>(base_y - h * 57 / 100)},
                         {static_cast<short>(x + w * 98 / 100), static_cast<short>(base_y - h * 28 / 100)},
                         {static_cast<short>(x + w * 89 / 100), static_cast<short>(base_y - h * 18 / 100)},
                         {static_cast<short>(x + w * 8 / 100), static_cast<short>(base_y - h * 18 / 100)}};
        XFillPolygon(dpy_, canvas(), gc_, body, static_cast<int>(std::size(body)), Complex, CoordModeOrigin);
        fill({x + w * 7 / 100, base_y - h * 55 / 100, w * 86 / 100, std::max(4, h / 18)}, 235, 224, 188);
        fill({x + w * 37 / 100, base_y - h * 90 / 100, w * 13 / 100, h * 22 / 100}, 44, 69, 79);
        fill({x + w * 52 / 100, base_y - h * 90 / 100, w * 13 / 100, h * 22 / 100}, 38, 61, 70);
        line(x + w * 51 / 100, base_y - h * 92 / 100, x + w * 51 / 100, base_y - h * 22 / 100, 32, 28, 27, 2);
        line(x + w * 68 / 100, base_y - h * 67 / 100, x + w * 68 / 100, base_y - h * 23 / 100, 32, 28, 27, 2);
        fill({x + w * 91 / 100, base_y - h * 50 / 100, w * 5 / 100, h * 14 / 100}, 242, 219, 135);
        fill({x + w * 4 / 100, base_y - h * 48 / 100, w * 4 / 100, h * 12 / 100}, 154, 28, 24);
        fill({x + w * 10 / 100, base_y - h * 23 / 100, w * 80 / 100, std::max(4, h / 18)}, 198, 197, 185);
        wheel(rear_x, wheel_y, wheel_r); wheel(front_x, wheel_y, wheel_r);
        line(x + w * 2 / 100, base_y - 2, x + w * 99 / 100, base_y - 2, 20, 18, 17, 3);
    }

    void draw_topdown_sprite(const assets::EmbeddedSprite& sprite, int cx, int base_y, int target_width) {
        const int scale = std::max(1, target_width / sprite.width);
        const int x0 = cx - sprite.width * scale / 2, y0 = base_y - sprite.height * scale;
        XRectangle batch[256];
        for (std::size_t ri = 0; ri < sprite.range_count; ++ri) {
            const auto& range = sprite.ranges[ri]; color(range.color.r, range.color.g, range.color.b);
            std::size_t consumed = 0;
            while (consumed < range.count) {
                const std::size_t n = std::min<std::size_t>(std::size(batch), range.count - consumed);
                for (std::size_t i = 0; i < n; ++i) {
                    const auto& src = sprite.rects[range.offset + consumed + i];
                    batch[i].x = static_cast<short>(x0 + src.x * scale); batch[i].y = static_cast<short>(y0 + src.y * scale);
                    batch[i].width = static_cast<unsigned short>(src.w * scale); batch[i].height = static_cast<unsigned short>(src.h * scale);
                }
                XFillRectangles(dpy_, canvas(), gc_, batch, static_cast<int>(n)); consumed += n;
            }
        }
    }

    void draw_workshop(bool darken = false) {
        fill({0, 0, width_, height_}, 25, 23, 22);
        brick_wall(64, height_ * 70 / 100);
        fill({0, height_ * 70 / 100, width_, height_ * 30 / 100}, 58, 56, 52);
        for (int y = height_ * 72 / 100; y < height_; y += 28) line(0, y, width_, y, 48, 46, 43);
        fill({width_ - 315, 102, 245, 165}, 45, 42, 38); outline({width_ - 315, 102, 245, 165}, 121, 104, 84, 3);
        text(width_ - 285, 128, "PEGBOARD", 211, 188, 150);
        for (int i = 0; i < 6; ++i) {
            line(width_ - 280 + i * 34, 150, width_ - 280 + i * 34, 213 + (i % 2) * 18, 180, 176, 165, 4);
            line(width_ - 290 + i * 34, 160 + (i % 3) * 14, width_ - 268 + i * 34, 160 + (i % 3) * 14, 180, 176, 165, 3);
        }
        fill({width_ - 350, 283, 300, 24}, 91, 66, 43); fill({width_ - 335, 307, 26, 110}, 66, 48, 34); fill({width_ - 90, 307, 26, 110}, 66, 48, 34);
        fill({45, 115, 235, 145}, 39, 38, 36); outline({45, 115, 235, 145}, 145, 132, 111, 2);
        text(67, 142, "BACKYARD SPEED SHOP", 231, 210, 167);
        fill({70, 162, 80, 58}, 115, 24, 25); text(82, 198, "HOT ROD", 244, 220, 162);
        fill({170, 163, 80, 57}, 25, 74, 99); text(184, 198, "RACE", 226, 231, 216);
        fill({330, 100, 360, 24}, 225, 219, 189); fill({350, 106, 320, 8}, 246, 243, 220);
        fill({320, 164, 190, 80}, 71, 67, 61); outline({320, 164, 190, 80}, 130, 124, 112, 2);
        text(341, 190, "PARTS SHELF", 221, 207, 180);
        for (int i = 0; i < 4; ++i) fill({340 + i * 40, 207, 27, 23}, 95 + i * 24, 45, 35 + i * 13);
        fill({70, height_ * 70 / 100 - 28, 80, 28}, 110, 22, 24); fill({91, height_ * 70 / 100 - 55, 38, 27}, 91, 91, 86);
        wheel(width_ - 180, height_ * 70 / 100 - 12, 30); wheel(width_ - 118, height_ * 70 / 100 - 12, 30);
        if (darken) fill({0, 64, width_, height_ - 64}, 0, 0, 0);
    }

    void draw_menu() {
        draw_workshop(false);
        draw_side_car(width_ * 70 / 100, height_ * 76 / 100, std::min(680, width_ * 52 / 100), 142, 31, 31);
        fill({36, 84, 455, 520}, 12, 12, 12); outline({36, 84, 455, 520}, 196, 165, 118, 3);
        big_text(68, 130, "BACKYARD RACER", 246, 218, 168);
        text(70, 165, "BUILD IT  -  RACE IT  -  RISK IT", 215, 203, 177);
        text(70, 200, "A HOT-ROD SUMMER IN YOUR OWN GARAGE", 166, 157, 143);
        button(70, 252, 330, 48, "NEW GAME");
        button(70, 314, 330, 48, "CONTINUE", game_.started());
        button(70, 376, 330, 48, "SETTINGS");
        button(70, 438, 330, 48, "QUIT");
        text(70, 555, message_, 193, 181, 158);
        text(width_ - 335, height_ - 20, "SCENE-BASED SIDE VIEW PRESENTATION", 218, 196, 159);
    }

    void draw_classifieds() {
        fill({0, 0, width_, height_}, 205, 196, 166);
        fill({20, 20, width_ - 40, height_ - 40}, 232, 224, 195); outline({20, 20, width_ - 40, height_ - 40}, 37, 34, 29, 3);
        big_text(48, 56, "THE BACKYARD GAZETTE", 28, 26, 23);
        line(48, 69, width_ - 48, 69, 38, 35, 30, 3);
        text(48, 92, "USED CARS  -  CASH TALKS  -  CLICK AN AD TO BUY", 48, 44, 38);
        const auto& cars = game_.classifieds();
        const int gap = 14, left = 48, top = 112, card_w = (width_ - 110) / 2, card_h = 112;
        for (std::size_t i = 0; i < cars.size(); ++i) {
            const int col = static_cast<int>(i % 2), row = static_cast<int>(i / 2);
            Rect card{left + col * (card_w + gap), top + row * (card_h + 10), card_w, card_h};
            const bool hover = card.contains(mouse_x_, mouse_y_);
            if (hover) fill(card, 246, 238, 206);
            outline(card, 61, 55, 45, hover ? 3 : 1);
            big_text(card.x + 12, card.y + 25, car_display_name(cars[i]), 35, 32, 27);
            text(card.x + 12, card.y + 50, "$" + std::to_string(cars[i].price) + "   " + std::to_string(cars[i].horsepower) + " HP   " + std::to_string(cars[i].weight_lb) + " LB", 55, 50, 43);
            text(card.x + 12, card.y + 75, "OWNER SAYS: RUNS STRONG - COME SEE IT", 76, 68, 57);
            const unsigned rr = static_cast<unsigned>(90 + (i * 37) % 125), gg = static_cast<unsigned>(30 + (i * 19) % 80), bb = static_cast<unsigned>(35 + (i * 53) % 145);
            draw_side_car(card.x + card.w - 98, card.y + card.h - 10, 165, rr, gg, bb);
        }
        button(width_ - 205, height_ - 62, 160, 36, "GARAGE");
        text(48, height_ - 38, message_, 70, 62, 50);
    }

    void draw_garage() {
        draw_workshop(false); status_bar("BACKYARD GARAGE");
        const OwnedCar* car = game_.active_car();
        if (car) {
            big_text(38, 96, car_display_name(car->base), 245, 222, 180);
            text(38, 121, "HP " + std::to_string(car->horsepower()) + "   WEIGHT " + std::to_string(car->base.weight_lb) + " LB   CONDITION " + std::to_string(car->condition) + "%", 233, 220, 195);
            text(38, 146, "SELL $" + std::to_string(car->resale_value()) + "   REPAIR $" + std::to_string(car->repair_cost()) + "   SPARES " + std::to_string(game_.spare_parts().size()), 221, 207, 180);
            button(38, 166, 138, 34, "REPAIR", car->repair_cost() > 0); button(190, 166, 138, 34, "SELL CAR");
            draw_side_car(width_ / 2, height_ * 76 / 100, std::min(760, width_ * 58 / 100), 145, 32, 32);
            fill({width_ - 360, 324, 320, 142}, 26, 24, 22); outline({width_ - 360, 324, 320, 142}, 185, 157, 118, 2);
            text(width_ - 337, 350, "INSTALLED", 241, 219, 179);
            int y = 376;
            if (car->installed_parts.empty()) text(width_ - 337, y, "STOCK HARDWARE", 207, 196, 177);
            else for (const auto& p : car->installed_parts) { text(width_ - 337, y, part_type_name(p.type) + ": " + p.name, 207, 196, 177); y += 20; if (y > 450) break; }
        } else {
            big_text(40, 120, "EMPTY GARAGE", 245, 222, 180); text(40, 150, "GRAB THE PAPER AND BUY YOUR FIRST CAR.", 223, 208, 183);
        }
        button(30, height_ - 57, 174, 38, "CLASSIFIEDS");
        button(216, height_ - 57, 154, 38, "PARTS / TUNE", car != nullptr);
        button(382, height_ - 57, 148, 38, "DRIVE-IN", car != nullptr);
        button(542, height_ - 57, 150, 38, "NEXT CAR", game_.garage().size() > 1);
        button(width_ - 180, height_ - 57, 150, 38, "MAIN MENU");
        text(32, height_ - 76, message_, 235, 220, 190);
    }

    Rect catalog_rect(std::size_t i) const {
        const int split = width_ * 63 / 100, left = 28, top = 128, gap = 10;
        const int card_w = (split - 68) / 2, card_h = 65;
        return {left + static_cast<int>(i % 2) * (card_w + gap), top + static_cast<int>(i / 2) * (card_h + gap), card_w, card_h};
    }
    Rect spare_rect(std::size_t i) const {
        const int split = width_ * 63 / 100, x = split + 22, y = 128 + static_cast<int>(i) * 58;
        return {x, y, width_ - x - 28, 46};
    }

    void draw_parts() {
        draw_workshop(false); status_bar("GARAGE - HOOD UP / PARTS BENCH");
        fill({18, 84, width_ * 64 / 100, height_ - 150}, 18, 18, 17); outline({18, 84, width_ * 64 / 100, height_ - 150}, 151, 126, 94, 3);
        fill({width_ * 64 / 100 + 12, 84, width_ * 36 / 100 - 30, height_ - 150}, 26, 23, 20); outline({width_ * 64 / 100 + 12, 84, width_ * 36 / 100 - 30, height_ - 150}, 151, 126, 94, 3);
        text(30, 111, "NEW HARDWARE - CLICK TO BUY + INSTALL", 236, 215, 176);
        const int split = width_ * 63 / 100;
        text(split + 24, 111, "PARTS ON YOUR SHELF", 236, 215, 176);
        const auto& parts = game_.parts_catalog();
        for (std::size_t i = 0; i < parts.size(); ++i) {
            Rect r = catalog_rect(i); const bool hover = r.contains(mouse_x_, mouse_y_);
            fill(r, hover ? 88U : 38U, hover ? 32U : 34U, hover ? 28U : 31U); outline(r, 130, 112, 91);
            text(r.x + 9, r.y + 21, parts[i].name, 240, 224, 194);
            text(r.x + 9, r.y + 45, part_type_name(parts[i].type) + "  $" + std::to_string(parts[i].price) + "  +" + std::to_string(parts[i].horsepower_gain) + " HP", 194, 183, 165);
        }
        const auto& spares = game_.spare_parts();
        if (spares.empty()) text(split + 24, 150, "EMPTY SHELF", 180, 169, 151);
        else for (std::size_t i = 0; i < spares.size() && i < 7; ++i) {
            Rect r = spare_rect(i); const bool hover = r.contains(mouse_x_, mouse_y_);
            fill(r, hover ? 83U : 48U, hover ? 57U : 42U, hover ? 35U : 34U); outline(r, 142, 122, 98);
            text(r.x + 9, r.y + 19, spares[i].name, 239, 223, 193); text(r.x + 9, r.y + 38, part_type_name(spares[i].type) + " - REINSTALL FREE", 191, 179, 158);
        }
        button(width_ - 180, height_ - 55, 150, 36, "DONE"); text(30, height_ - 70, message_, 231, 214, 182);
    }

    void draw_diner() {
        fill({0, 0, width_, height_}, 20, 28, 43);
        fill({0, height_ * 54 / 100, width_, height_ * 46 / 100}, 30, 31, 34);
        for (int i = 0; i < 24; ++i) fill({i * 70 % width_, 82 + (i * 47) % 190, 2, 2}, 220, 220, 190);
        fill({width_ * 18 / 100, 145, width_ * 64 / 100, 255}, 74, 55, 46);
        fill({width_ * 20 / 100, 175, width_ * 60 / 100, 190}, 216, 205, 174);
        for (int i = 0; i < 6; ++i) fill({width_ * 22 / 100 + i * (width_ * 9 / 100), 215, width_ * 7 / 100, 100}, 30, 72, 86);
        fill({width_ * 15 / 100, 110, width_ * 70 / 100, 74}, 23, 18, 21); outline({width_ * 15 / 100, 110, width_ * 70 / 100, 74}, 225, 54, 83, 4);
        big_text(width_ * 27 / 100, 155, "BACKYARD DRIVE-IN", 245, 85, 118);
        fill({0, 400, width_, 12}, 151, 140, 112); line(0, 480, width_, 480, 229, 218, 175, 3);
        status_bar("DRIVE-IN - FIND A RACE");
        const Opponent opp = game_.current_opponent();
        draw_side_car(width_ * 28 / 100, height_ * 76 / 100, std::min(470, width_ * 37 / 100), 45, 98, 139);
        draw_side_car(width_ * 72 / 100, height_ * 76 / 100, std::min(470, width_ * 37 / 100), 157, 40, 32);
        fill({45, 84, 390, 88}, 17, 16, 17); outline({45, 84, 390, 88}, 197, 164, 116, 2);
        big_text(65, 112, opp.name + " WANTS TO RACE", 243, 220, 179);
        text(65, 138, car_display_name(opp.car.base) + "   " + std::to_string(opp.car.horsepower()) + " HP", 210, 198, 176);
        text(65, 160, "YOUR REP " + std::to_string(game_.reputation()) + "   RECORD " + std::to_string(game_.wins()) + "-" + std::to_string(game_.losses()), 194, 183, 165);
        button(70, height_ - 61, 175, 40, "DRAG - $100"); button(260, height_ - 61, 175, 40, "DRAG - $250"); button(450, height_ - 61, 190, 40, "RACE FOR PINKS");
        button(width_ - 180, height_ - 61, 150, 40, "GARAGE");
    }

    void draw_race() {
        fill({0, 0, width_, height_}, 14, 18, 24); status_bar("QUARTER MILE - YOU ARE DRIVING");
        const int start_x = 90, finish_x = width_ - 90, road_w = std::max(1, finish_x - start_x);
        const double pf = std::clamp(race_.distance_ft() / 1320.0, 0.0, 1.0), of = std::clamp(race_.opponent_distance_ft() / 1320.0, 0.0, 1.0);
        const int px = start_x + static_cast<int>(pf * road_w), ox = start_x + static_cast<int>(of * road_w);
        fill({55, 155, width_ - 110, 330}, 46, 47, 49); fill({55, 155, width_ - 110, 42}, 35, 36, 38); fill({55, 443, width_ - 110, 42}, 35, 36, 38);
        line(start_x, 175, start_x, 465, 225, 225, 210); line(finish_x, 175, finish_x, 465, 225, 225, 210);
        for (int x = start_x; x < finish_x; x += 70) line(x, 322, std::min(x + 35, finish_x), 322, 145, 145, 135);
        draw_topdown_sprite(assets::kenney_red_car, px, 300, 150); draw_topdown_sprite(assets::kenney_blue_car, ox, 445, 150);
        text(65, 202, "YOU"); text(65, 350, race_opponent_.name, 220, 220, 210);
        const bool green = race_.green(); fill({width_ / 2 - 42, 95, 84, 42}, green ? 28U : 180U, green ? 178U : 35U, 35U); big_text(width_ / 2 - 30, 122, green ? "GO" : "RED");
        std::ostringstream speed, distance; speed << std::fixed << std::setprecision(1) << race_.speed_mph(); distance << std::fixed << std::setprecision(0) << race_.distance_ft();
        text(65, 520, "SPEED " + speed.str() + " MPH   RPM " + std::to_string(race_.rpm()) + "   GEAR " + std::to_string(race_.gear()) + "/" + std::to_string(race_.max_gears()));
        text(65, 548, "DISTANCE " + distance.str() + " / 1320 FT", 210, 212, 205);
        const int tx = 65, ty = 575, tw = width_ - 130; fill({tx, ty, tw, 20}, 25, 25, 25);
        const int tf = static_cast<int>(std::clamp(race_.rpm() / 7600.0, 0.0, 1.0) * tw); fill({tx, ty, tf, 20}, race_.rpm() >= 6500 ? 205U : 185U, race_.rpm() >= 6500 ? 55U : 160U, 45U); outline({tx, ty, tw, 20}, 190, 192, 194);
        text(65, 630, "HOLD UP / W / SPACE = THROTTLE    A = UPSHIFT    Z = DOWNSHIFT", 225, 225, 214);
        text(65, 656, green ? "SHIFT NEAR THE REDLINE - YOUR ET DECIDES THE BET" : "STAGE IT: HOLD THROTTLE BEFORE GREEN", 190, 192, 194);
    }

    void draw_result() {
        draw_workshop(false); status_bar("RACE RESULT");
        fill({65, 105, width_ - 130, 350}, 16, 15, 14); outline({65, 105, width_ - 130, 350}, 186, 157, 116, 3);
        const bool won = last_race_.won; big_text(95, 155, last_race_.summary, won ? 125U : 225U, won ? 210U : 95U, won ? 125U : 80U);
        if (last_race_.valid) {
            std::ostringstream p, o; p << std::fixed << std::setprecision(2) << last_race_.player_et; o << std::fixed << std::setprecision(2) << last_race_.opponent_et;
            text(95, 210, "YOUR ET      " + p.str() + " SEC"); text(95, 240, "OPPONENT ET  " + o.str() + " SEC");
            if (!last_race_.pink_slip) text(95, 275, "CASH CHANGE  " + std::to_string(last_race_.cash_delta));
            text(95, 305, "REPUTATION   " + std::to_string(last_race_.reputation_delta)); text(95, 335, "RACE WEAR    -" + std::to_string(last_race_.wear) + "% CONDITION");
        }
        button(95, 382, 220, 48, "BACK TO GARAGE");
    }

    void draw_settings() {
        draw_workshop(false); status_bar("SETTINGS"); fill({70, 115, 520, 190}, 18, 17, 16); outline({70, 115, 520, 190}, 178, 149, 110, 2);
        big_text(95, 155, "SETTINGS", 240, 218, 178); text(95, 190, "GAMEPLAY FIRST. MORE DISPLAY AND AUDIO OPTIONS ARE COMING.", 200, 187, 165); button(95, 235, 180, 40, "MAIN MENU");
    }

    void draw() {
        ensure_back_buffer();
        switch (screen_) {
            case Screen::Menu: draw_menu(); break; case Screen::Classifieds: draw_classifieds(); break; case Screen::Garage: draw_garage(); break;
            case Screen::Parts: draw_parts(); break; case Screen::Diner: draw_diner(); break; case Screen::Race: draw_race(); break;
            case Screen::Result: draw_result(); break; case Screen::Settings: draw_settings(); break;
        }
        XCopyArea(dpy_, back_buffer_, win_, gc_, 0, 0, static_cast<unsigned>(std::max(1, width_)), static_cast<unsigned>(std::max(1, height_)), 0, 0); XFlush(dpy_);
    }

    void start_race(int wager, bool pinks) {
        const OwnedCar* car = game_.active_car(); if (!car) { message_ = "BUY A CAR BEFORE YOU RACE"; return; }
        if (!pinks && game_.cash() < wager) { message_ = "YOU CANNOT COVER THAT BET"; return; }
        race_opponent_ = game_.current_opponent(); race_wager_ = wager; race_pinks_ = pinks; race_.start(*car, race_opponent_);
        if (!infiltratr_fixed_step_configure(&race_clock_, 1000000000ULL, 60ULL, 250000000ULL, 8ULL) || !infiltratr_fixed_step_reset(&race_clock_, monotonic_ns()))
            throw std::runtime_error("Common fixed-step scheduler could not initialize");
        race_clock_ready_ = true; message_ = "STAGE - HOLD THROTTLE, THEN SHIFT IT YOURSELF"; screen_ = Screen::Race;
    }

    bool tick_race() {
        if (screen_ != Screen::Race || !race_clock_ready_) return false;
        InfiltratrFixedStepResult timing{}; if (!infiltratr_fixed_step_advance(&race_clock_, monotonic_ns(), &timing)) throw std::runtime_error("Common fixed-step scheduler failed during race");
        if (timing.steps_to_run == 0) return false;
        for (std::uint64_t s = 0; s < timing.steps_to_run && screen_ == Screen::Race; ++s) {
            race_.update(1.0 / 60.0);
            if (race_.finished()) { last_race_ = game_.settle_race(race_opponent_, race_wager_, race_pinks_, race_.player_et()); message_ = last_race_.summary; race_clock_ready_ = false; screen_ = Screen::Result; }
        }
        return true;
    }

    static bool throttle_key(KeySym k) { return k == XK_Up || k == XK_w || k == XK_W || k == XK_space; }
    void key_press(KeySym k) {
        if (screen_ == Screen::Race) {
            if (throttle_key(k)) race_.set_throttle(true); else if (k == XK_a || k == XK_A || k == XK_Right) race_.shift_up(); else if (k == XK_z || k == XK_Z || k == XK_Left) race_.shift_down();
            else if (k == XK_Escape) { race_.set_throttle(false); race_clock_ready_ = false; message_ = "RACE ABORTED"; screen_ = Screen::Diner; }
            return;
        }
        if (k != XK_Escape) return;
        if (screen_ == Screen::Menu) running_ = false; else if (screen_ == Screen::Garage) screen_ = Screen::Menu;
        else if (screen_ == Screen::Result || screen_ == Screen::Parts || screen_ == Screen::Diner || screen_ == Screen::Classifieds) screen_ = Screen::Garage; else screen_ = Screen::Menu;
    }
    void key_release(KeySym k) { if (screen_ == Screen::Race && throttle_key(k)) race_.set_throttle(false); }

    void click(int x, int y) {
        if (screen_ == Screen::Menu) {
            if (Rect{70,252,330,48}.contains(x,y)) { game_.new_game(); message_ = "PICK YOUR FIRST CAR"; screen_ = Screen::Classifieds; }
            else if (Rect{70,314,330,48}.contains(x,y) && game_.started()) screen_ = game_.garage().empty() ? Screen::Classifieds : Screen::Garage;
            else if (Rect{70,376,330,48}.contains(x,y)) screen_ = Screen::Settings; else if (Rect{70,438,330,48}.contains(x,y)) running_ = false; return;
        }
        if (screen_ == Screen::Classifieds) {
            const int gap=14,left=48,top=112,card_w=(width_-110)/2,card_h=112; const auto& cars=game_.classifieds();
            for (std::size_t i=0;i<cars.size();++i) { Rect r{left+static_cast<int>(i%2)*(card_w+gap),top+static_cast<int>(i/2)*(card_h+10),card_w,card_h}; if (r.contains(x,y)) { std::string e; if(game_.buy_car(i,&e)){message_="CAR BOUGHT - IT IS IN YOUR GARAGE";screen_=Screen::Garage;}else message_=e; return; } }
            if(Rect{width_-205,height_-62,160,36}.contains(x,y)) screen_=Screen::Garage; return;
        }
        if (screen_ == Screen::Garage) {
            if(game_.active_car()&&Rect{38,166,138,34}.contains(x,y)){int cost=0;std::string e;if(game_.repair_active_car(&cost,&e))message_="CAR REPAIRED FOR $"+std::to_string(cost);else message_=e;return;}
            if(game_.active_car()&&Rect{190,166,138,34}.contains(x,y)){int price=0;std::string e;if(game_.sell_active_car(&price,&e)){message_="CAR SOLD FOR $"+std::to_string(price);if(game_.garage().empty())screen_=Screen::Classifieds;}else message_=e;return;}
            if(Rect{30,height_-57,174,38}.contains(x,y))screen_=Screen::Classifieds;else if(Rect{216,height_-57,154,38}.contains(x,y)&&game_.active_car())screen_=Screen::Parts;else if(Rect{382,height_-57,148,38}.contains(x,y)&&game_.active_car())screen_=Screen::Diner;else if(Rect{542,height_-57,150,38}.contains(x,y)&&game_.garage().size()>1){game_.next_car();message_="ACTIVE CAR CHANGED";}else if(Rect{width_-180,height_-57,150,38}.contains(x,y))screen_=Screen::Menu;return;
        }
        if(screen_==Screen::Parts){const auto& parts=game_.parts_catalog();for(std::size_t i=0;i<parts.size();++i)if(catalog_rect(i).contains(x,y)){std::string e;if(game_.buy_part(i,&e))message_=parts[i].name+" INSTALLED";else message_=e;return;}const auto n=game_.spare_parts().size();for(std::size_t i=0;i<n&&i<7;++i)if(spare_rect(i).contains(x,y)){const std::string name=game_.spare_parts()[i].name;std::string e;if(game_.install_spare(i,&e))message_=name+" REINSTALLED";else message_=e;return;}if(Rect{width_-180,height_-55,150,36}.contains(x,y))screen_=Screen::Garage;return;}
        if(screen_==Screen::Diner){if(Rect{70,height_-61,175,40}.contains(x,y))start_race(100,false);else if(Rect{260,height_-61,175,40}.contains(x,y))start_race(250,false);else if(Rect{450,height_-61,190,40}.contains(x,y))start_race(0,true);else if(Rect{width_-180,height_-61,150,40}.contains(x,y))screen_=Screen::Garage;return;}
        if(screen_==Screen::Result){if(Rect{95,382,220,48}.contains(x,y))screen_=game_.garage().empty()?Screen::Classifieds:Screen::Garage;return;}
        if(screen_==Screen::Settings&&Rect{95,235,180,40}.contains(x,y))screen_=Screen::Menu;
    }
};

} // namespace backyard_racer

int main(int argc, char** argv) {
    const InfiltratrProjectInfo& info = backyard_racer::project_info();
    if (!infiltratr_project_info_is_valid(&info)) { std::cerr << "Backyard Racer project metadata is invalid\n"; return 1; }
    if (argc == 2 && infiltratr_string_equal(argv[1], "--version")) { std::cout << info.program_name << ' ' << info.version << '\n'; return 0; }
    if (argc == 2 && infiltratr_string_equal(argv[1], "--project-info")) return infiltratr_project_info_print(stdout, &info) == 0 ? 0 : 1;
    try { backyard_racer::App app(1280, 720); return app.run(); }
    catch (const std::exception& e) { std::cerr << info.program_name << " failed to start: " << e.what() << '\n'; return 1; }
}
