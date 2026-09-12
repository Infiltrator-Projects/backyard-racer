#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace backyard_racer {

struct Color { std::uint8_t r, g, b; };
struct Rect {
    int x, y, w, h;
    bool contains(int px, int py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};

enum class Screen { Menu, Garage };

class Framebuffer {
public:
    Framebuffer(int w, int h) { resize(w, h); }

    void resize(int w, int h) {
        width_ = std::max(1, w);
        height_ = std::max(1, h);
        pixels_.assign(static_cast<std::size_t>(width_ * height_), Color{0, 0, 0});
    }

    int width() const { return width_; }
    int height() const { return height_; }
    const std::vector<Color>& pixels() const { return pixels_; }

    void clear(Color c) { std::fill(pixels_.begin(), pixels_.end(), c); }

    void pixel(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
        pixels_[static_cast<std::size_t>(y * width_ + x)] = c;
    }

    void fill_rect(Rect r, Color c) {
        const int x0 = std::max(0, r.x);
        const int y0 = std::max(0, r.y);
        const int x1 = std::min(width_, r.x + r.w);
        const int y1 = std::min(height_, r.y + r.h);
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                pixel(x, y, c);
    }

    void blend_rect(Rect r, Color c, std::uint8_t alpha) {
        const int x0 = std::max(0, r.x);
        const int y0 = std::max(0, r.y);
        const int x1 = std::min(width_, r.x + r.w);
        const int y1 = std::min(height_, r.y + r.h);
        const unsigned a = alpha;
        const unsigned ia = 255U - a;
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                Color& dst = pixels_[static_cast<std::size_t>(y * width_ + x)];
                dst.r = static_cast<std::uint8_t>((dst.r * ia + c.r * a) / 255U);
                dst.g = static_cast<std::uint8_t>((dst.g * ia + c.g * a) / 255U);
                dst.b = static_cast<std::uint8_t>((dst.b * ia + c.b * a) / 255U);
            }
        }
    }

    void rect(Rect r, Color c, int thickness = 1) {
        fill_rect({r.x, r.y, r.w, thickness}, c);
        fill_rect({r.x, r.y + r.h - thickness, r.w, thickness}, c);
        fill_rect({r.x, r.y, thickness, r.h}, c);
        fill_rect({r.x + r.w - thickness, r.y, thickness, r.h}, c);
    }

    void line(int x0, int y0, int x1, int y1, Color c) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        while (true) {
            pixel(x0, y0, c);
            if (x0 == x1 && y0 == y1) break;
            const int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void triangle(int x0, int y0, int x1, int y1, int x2, int y2, Color c) {
        const int min_y = std::max(0, std::min({y0, y1, y2}));
        const int max_y = std::min(height_ - 1, std::max({y0, y1, y2}));
        const int min_x = std::max(0, std::min({x0, x1, x2}));
        const int max_x = std::min(width_ - 1, std::max({x0, x1, x2}));
        auto edge = [](int ax, int ay, int bx, int by, int px, int py) {
            return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
        };
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                const int e0 = edge(x0, y0, x1, y1, x, y);
                const int e1 = edge(x1, y1, x2, y2, x, y);
                const int e2 = edge(x2, y2, x0, y0, x, y);
                if ((e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0))
                    pixel(x, y, c);
            }
        }
    }

private:
    int width_ = 1;
    int height_ = 1;
    std::vector<Color> pixels_;
};

using Glyph = std::array<std::uint8_t, 7>;

const std::unordered_map<char, Glyph> FONT = {
    {' ',{0,0,0,0,0,0,0}}, {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
    {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}}, {'E',{31,16,16,30,16,16,31}},
    {'F',{31,16,16,30,16,16,16}}, {'G',{14,17,16,23,17,17,14}}, {'H',{17,17,17,31,17,17,17}},
    {'I',{14,4,4,4,4,4,14}}, {'J',{1,1,1,1,17,17,14}}, {'K',{17,18,20,24,20,18,17}},
    {'L',{16,16,16,16,16,16,31}}, {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
    {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}}, {'Q',{14,17,17,17,21,18,13}},
    {'R',{30,17,17,30,20,18,17}}, {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
    {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}}, {'W',{17,17,17,21,21,21,10}},
    {'X',{17,17,10,4,10,17,17}}, {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
    {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}}, {'2',{14,17,1,2,4,8,31}},
    {'3',{30,1,1,14,1,1,30}}, {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
    {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}}, {'8',{14,17,17,14,17,17,14}},
    {'9',{14,17,17,15,1,1,14}}, {'-',{0,0,0,31,0,0,0}}, {'.',{0,0,0,0,0,12,12}},
    {':',{0,12,12,0,12,12,0}}, {'/',{1,2,2,4,8,8,16}}, {'!',{4,4,4,4,4,0,4}}
};

void draw_text(Framebuffer& fb, int x, int y, const std::string& text, Color c, int scale = 2) {
    int cursor = x;
    for (char raw : text) {
        const char ch = raw >= 'a' && raw <= 'z' ? static_cast<char>(raw - 'a' + 'A') : raw;
        const auto it = FONT.find(ch);
        const Glyph& glyph = it != FONT.end() ? it->second : FONT.at(' ');
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if ((glyph[row] >> (4 - col)) & 1U)
                    fb.fill_rect({cursor + col * scale, y + row * scale, scale, scale}, c);
        cursor += 6 * scale;
    }
}

class Game {
public:
    explicit Game(Framebuffer& fb) : fb_(fb) {}

    bool running() const { return running_; }
    bool dirty() const { return dirty_; }
    void rendered() { dirty_ = false; }
    void resize() { dirty_ = true; }

    void on_key(KeySym key) {
        if (key != XK_Escape) return;
        if (screen_ == Screen::Garage) screen_ = Screen::Menu;
        else running_ = false;
        dirty_ = true;
    }

    void on_motion(int x, int y) {
        mouse_x_ = x;
        mouse_y_ = y;
        const int old = hover_;
        hover_ = screen_ == Screen::Menu ? button_at(x, y) : -1;
        if (old != hover_) dirty_ = true;
    }

    void on_click(int x, int y) {
        if (screen_ == Screen::Garage) {
            if (back_rect().contains(x, y)) {
                screen_ = Screen::Menu;
                dirty_ = true;
            }
            return;
        }

        const int button = button_at(x, y);
        if (button == 0) {
            screen_ = Screen::Garage;
            status_.clear();
        } else if (button == 2) {
            status_ = "SETTINGS ARE NOT WIRED YET";
        } else if (button == 3) {
            running_ = false;
        }
        dirty_ = true;
    }

    void draw() {
        if (screen_ == Screen::Menu) draw_menu();
        else draw_garage();
    }

private:
    Framebuffer& fb_;
    Screen screen_ = Screen::Menu;
    bool running_ = true;
    bool dirty_ = true;
    int mouse_x_ = 0;
    int mouse_y_ = 0;
    int hover_ = -1;
    std::string status_ = "PRE-ALPHA - STREET ROD SHELL";

    const Color chrome{198, 205, 211};
    const Color pale{235, 236, 228};
    const Color panel{16, 17, 19};
    const Color panel_hi{48, 51, 55};
    const Color asphalt{46, 47, 49};
    const Color concrete{101, 96, 88};
    const Color red{178, 35, 38};

    std::array<Rect, 4> menu_buttons() const {
        const int x = 72, y = 300, w = 340, h = 50, gap = 12;
        return {Rect{x,y,w,h}, Rect{x,y+h+gap,w,h}, Rect{x,y+2*(h+gap),w,h}, Rect{x,y+3*(h+gap),w,h}};
    }

    Rect back_rect() const { return {fb_.width() - 250, 24, 220, 42}; }

    int button_at(int x, int y) const {
        const auto buttons = menu_buttons();
        for (int i = 0; i < 4; ++i)
            if (buttons[i].contains(x, y)) return i;
        return -1;
    }

    void button(Rect r, const std::string& label, bool enabled, bool hovered = false) {
        fb_.blend_rect(r, hovered && enabled ? panel_hi : panel, hovered && enabled ? 225 : 205);
        fb_.rect(r, enabled ? chrome : Color{78,80,82}, 2);
        draw_text(fb_, r.x + 18, r.y + 16, label, enabled ? pale : Color{108,110,112}, 2);
    }

    void draw_menu_background() {
        const int w = fb_.width();
        const int h = fb_.height();

        fb_.clear({22, 27, 35});
        fb_.fill_rect({0, h * 42 / 100, w, h * 58 / 100}, {52, 48, 44});
        fb_.fill_rect({0, h * 70 / 100, w, h * 30 / 100}, asphalt);
        fb_.fill_rect({0, h * 69 / 100, w, 5}, {154, 124, 72});

        const int gx = w * 56 / 100;
        const int gy = h * 24 / 100;
        const int gw = w * 38 / 100;
        const int gh = h * 45 / 100;
        fb_.fill_rect({gx, gy, gw, gh}, {72, 70, 66});
        fb_.fill_rect({gx + gw * 9 / 100, gy + gh * 18 / 100, gw * 72 / 100, gh * 82 / 100}, {31, 31, 31});
        fb_.rect({gx, gy, gw, gh}, {116, 113, 105}, 3);
        fb_.fill_rect({gx + gw * 20 / 100, gy + 24, gw * 50 / 100, 5}, {234, 226, 180});

        const int car_y = gy + gh * 70 / 100;
        const int car_x = gx + gw * 14 / 100;
        const int car_w = gw * 70 / 100;
        const int car_h = std::max(44, gh * 18 / 100);
        fb_.fill_rect({car_x, car_y, car_w, car_h}, red);
        fb_.triangle(car_x + car_w * 22 / 100, car_y,
                     car_x + car_w * 38 / 100, car_y - car_h * 65 / 100,
                     car_x + car_w * 70 / 100, car_y, red);
        fb_.fill_rect({car_x + car_w * 37 / 100, car_y - car_h * 48 / 100,
                       car_w * 27 / 100, car_h * 42 / 100}, {37, 48, 56});
        const int wheel = std::max(16, car_h * 45 / 100);
        fb_.fill_rect({car_x + car_w * 14 / 100, car_y + car_h - wheel / 2, wheel, wheel / 2}, {10,10,10});
        fb_.fill_rect({car_x + car_w * 72 / 100, car_y + car_h - wheel / 2, wheel, wheel / 2}, {10,10,10});

        for (int i = 0; i < 6; ++i) {
            const int y = h * 78 / 100 + i * 30;
            const int mw = 70 + i * 18;
            fb_.fill_rect({w - 100 - mw, y, mw, 5}, {210, 190, 117});
        }
    }

    void draw_menu() {
        draw_menu_background();
        fb_.blend_rect({44, 46, 548, 548}, {7, 8, 9}, 118);
        draw_text(fb_, 72, 78, "BACKYARD RACER", chrome, 6);
        draw_text(fb_, 74, 155, "BUILD IT  RACE IT  RISK IT", pale, 3);
        fb_.fill_rect({72, 206, 470, 2}, chrome);

        const auto buttons = menu_buttons();
        button(buttons[0], "NEW GAME", true, hover_ == 0);
        button(buttons[1], "CONTINUE", false);
        button(buttons[2], "SETTINGS", true, hover_ == 2);
        button(buttons[3], "QUIT", true, hover_ == 3);

        if (!status_.empty()) draw_text(fb_, 72, 570, status_, {196,198,199}, 2);
    }

    void draw_garage() {
        const int w = fb_.width();
        const int h = fb_.height();

        fb_.clear(concrete);
        fb_.fill_rect({0, 0, w, 84}, panel);
        draw_text(fb_, 24, 20, "BACKYARD GARAGE", chrome, 3);
        draw_text(fb_, 24, 58, "CASH 2000   CARS 0   REPUTATION 0", pale, 2);

        const Rect back = back_rect();
        button(back, "MAIN MENU", true, back.contains(mouse_x_, mouse_y_));

        const int floor_y = h * 66 / 100;
        fb_.fill_rect({0, floor_y, w, h - floor_y}, {58, 57, 55});
        for (int x = 0; x < w; x += 80)
            fb_.line(x, floor_y, x + 120, h - 1, {76, 74, 71});

        const int bench_y = floor_y - 96;
        fb_.fill_rect({80, bench_y, 340, 26}, {94, 67, 45});
        fb_.fill_rect({105, bench_y + 26, 24, 112}, {66, 47, 33});
        fb_.fill_rect({365, bench_y + 26, 24, 112}, {66, 47, 33});

        draw_text(fb_, 92, bench_y - 42, "YOUR GARAGE", {45,42,39}, 3);
        draw_text(fb_, 92, bench_y + 58, "NO CAR YET", pale, 3);
        draw_text(fb_, 22, h - 34, "NEW GAME FLOW WILL START WITH THE CLASSIFIEDS", {38,36,34}, 2);
    }
};

class X11App {
public:
    X11App(int w, int h) : fb_(w, h), game_(fb_) {
        display_ = XOpenDisplay(nullptr);
        if (!display_) throw std::runtime_error("Unable to open X11 display");

        screen_ = DefaultScreen(display_);
        window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen_),
                                      100, 100, w, h, 0,
                                      BlackPixel(display_, screen_), BlackPixel(display_, screen_));
        XStoreName(display_, window_, "Backyard Racer");
        XSelectInput(display_, window_, ExposureMask | KeyPressMask | ButtonPressMask |
                                        PointerMotionMask | StructureNotifyMask);
        delete_window_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display_, window_, &delete_window_, 1);
        gc_ = XCreateGC(display_, window_, 0, nullptr);
        XMapWindow(display_, window_);
        recreate_image(w, h);
    }

    ~X11App() {
        if (image_) {
            std::free(image_->data);
            image_->data = nullptr;
            XDestroyImage(image_);
        }
        if (gc_) XFreeGC(display_, gc_);
        if (window_) XDestroyWindow(display_, window_);
        if (display_) XCloseDisplay(display_);
    }

    int run() {
        while (game_.running()) {
            XEvent event;
            XNextEvent(display_, &event);

            switch (event.type) {
                case Expose:
                    game_.resize();
                    break;
                case ConfigureNotify:
                    if (event.xconfigure.width != fb_.width() || event.xconfigure.height != fb_.height()) {
                        fb_.resize(event.xconfigure.width, event.xconfigure.height);
                        recreate_image(event.xconfigure.width, event.xconfigure.height);
                        game_.resize();
                    }
                    break;
                case MotionNotify:
                    game_.on_motion(event.xmotion.x, event.xmotion.y);
                    break;
                case ButtonPress:
                    if (event.xbutton.button == Button1)
                        game_.on_click(event.xbutton.x, event.xbutton.y);
                    break;
                case KeyPress:
                    game_.on_key(XLookupKeysym(&event.xkey, 0));
                    break;
                case ClientMessage:
                    if (static_cast<Atom>(event.xclient.data.l[0]) == delete_window_) return 0;
                    break;
                default:
                    break;
            }

            if (game_.dirty()) {
                game_.draw();
                present();
                game_.rendered();
            }
        }
        return 0;
    }

private:
    Display* display_ = nullptr;
    int screen_ = 0;
    Window window_ = 0;
    GC gc_ = 0;
    Atom delete_window_ = 0;
    XImage* image_ = nullptr;
    Framebuffer fb_;
    Game game_;

    static unsigned long pack(std::uint8_t value, unsigned long mask) {
        if (!mask) return 0;
        unsigned shift = 0;
        while (((mask >> shift) & 1UL) == 0UL) ++shift;
        const unsigned long max_value = mask >> shift;
        return ((static_cast<unsigned long>(value) * max_value + 127UL) / 255UL << shift) & mask;
    }

    void recreate_image(int w, int h) {
        if (image_) {
            std::free(image_->data);
            image_->data = nullptr;
            XDestroyImage(image_);
            image_ = nullptr;
        }

        image_ = XCreateImage(display_, DefaultVisual(display_, screen_), DefaultDepth(display_, screen_),
                              ZPixmap, 0, nullptr, w, h, 32, 0);
        if (!image_) throw std::runtime_error("Unable to create XImage");

        image_->data = static_cast<char*>(std::calloc(static_cast<std::size_t>(image_->bytes_per_line) * h, 1));
        if (!image_->data) throw std::bad_alloc();
    }

    void present() {
        const auto& pixels = fb_.pixels();
        const int w = fb_.width();
        const int h = fb_.height();

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const Color c = pixels[static_cast<std::size_t>(y * w + x)];
                const unsigned long packed = pack(c.r, image_->red_mask) |
                                             pack(c.g, image_->green_mask) |
                                             pack(c.b, image_->blue_mask);
                XPutPixel(image_, x, y, packed);
            }
        }

        XPutImage(display_, window_, gc_, image_, 0, 0, 0, 0, w, h);
        XFlush(display_);
    }
};

} // namespace backyard_racer

int main() {
    try {
        backyard_racer::X11App app(1280, 720);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Backyard Racer failed to start: " << e.what() << '\n';
        return 1;
    }
}
