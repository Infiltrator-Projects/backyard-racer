// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixel_app.hpp"
#include "platform.hpp"

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <infiltratr/timing.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <thread>

namespace backyard_racer::platform {
namespace {

PixelKey map_key(KeySym key) {
    switch (key) {
        case XK_Escape: return PixelKey::Escape;
        case XK_Up: return PixelKey::Up;
        case XK_Down: return PixelKey::Down;
        case XK_Left: return PixelKey::Left;
        case XK_Right: return PixelKey::Right;
        case XK_space: return PixelKey::Space;
        case XK_Return: return PixelKey::Enter;
        case XK_w: case XK_W: return PixelKey::W;
        case XK_a: case XK_A: return PixelKey::A;
        case XK_s: case XK_S: return PixelKey::S;
        case XK_d: case XK_D: return PixelKey::D;
        default: return PixelKey::Unknown;
    }
}

PixelMouseButton map_button(unsigned button) {
    switch (button) {
        case Button1: return PixelMouseButton::Left;
        case Button2: return PixelMouseButton::Middle;
        case Button3: return PixelMouseButton::Right;
        case Button4: return PixelMouseButton::WheelUp;
        case Button5: return PixelMouseButton::WheelDown;
        default: return PixelMouseButton::Unknown;
    }
}

class X11Host {
public:
    explicit X11Host(PixelApp& app) : app_(app) {
        display_ = XOpenDisplay(nullptr);
        if (!display_) throw std::runtime_error("Unable to open X11 display");

        screen_ = DefaultScreen(display_);
        visual_ = DefaultVisual(display_, screen_);
        window_ = XCreateSimpleWindow(
            display_, RootWindow(display_, screen_), 100, 100,
            static_cast<unsigned>(app_.width()), static_cast<unsigned>(app_.height()),
            0, BlackPixel(display_, screen_), BlackPixel(display_, screen_));
        XStoreName(display_, window_, "Backyard Racer");
        XSelectInput(display_, window_, ExposureMask | KeyPressMask | KeyReleaseMask |
            ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

        Bool detectable = False;
        XkbSetDetectableAutoRepeat(display_, True, &detectable);

        delete_atom_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display_, window_, &delete_atom_, 1);
        gc_ = XCreateGC(display_, window_, 0, nullptr);
        XMapWindow(display_, window_);
        recreate_image(app_.width(), app_.height());

        if (!infiltratr_fixed_step_configure(&scheduler_, 1000000000ULL, 60ULL,
                                              250000000ULL, 16ULL)) {
            throw std::runtime_error("Common fixed-step scheduler configuration failed");
        }
        infiltratr_fixed_step_reset(&scheduler_, now_ns());
    }

    ~X11Host() {
        destroy_image();
        if (gc_) XFreeGC(display_, gc_);
        if (window_) XDestroyWindow(display_, window_);
        if (display_) XCloseDisplay(display_);
    }

    int run() {
        constexpr std::uint64_t present_interval_ns = 1000000000ULL / 60ULL;
        while (app_.running()) {
            pump_events();

            const std::uint64_t now = now_ns();
            InfiltratrFixedStepResult result{};
            if (infiltratr_fixed_step_advance(&scheduler_, now, &result)) {
                for (std::uint64_t i = 0; i < result.steps_to_run; ++i) {
                    app_.tick(1.0 / 60.0);
                }
            }

            if (app_.dirty() &&
                (last_present_ns_ == 0 || now - last_present_ns_ >= present_interval_ns)) {
                app_.draw();
                present();
                app_.rendered();
                last_present_ns_ = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return 0;
    }

private:
    PixelApp& app_;
    Display* display_ = nullptr;
    int screen_ = 0;
    Visual* visual_ = nullptr;
    Window window_ = 0;
    GC gc_ = 0;
    Atom delete_atom_ = 0;
    XImage* image_ = nullptr;
    InfiltratrFixedStepScheduler scheduler_{};
    std::uint64_t last_present_ns_ = 0;
    std::array<std::uint32_t, 256> red_lut_{};
    std::array<std::uint32_t, 256> green_lut_{};
    std::array<std::uint32_t, 256> blue_lut_{};
    bool direct_32_ = false;

    static std::uint64_t now_ns() {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }

    static unsigned long pack_channel(std::uint8_t value, unsigned long mask) {
        if (!mask) return 0;
        unsigned shift = 0;
        while (((mask >> shift) & 1UL) == 0UL) ++shift;
        const unsigned long range = mask >> shift;
        return (((static_cast<unsigned long>(value) * range + 127UL) / 255UL) << shift) & mask;
    }

    void pump_events() {
        while (XPending(display_) > 0) {
            XEvent event{};
            XNextEvent(display_, &event);
            switch (event.type) {
                case Expose:
                    app_.resize(app_.width(), app_.height());
                    break;
                case ConfigureNotify:
                    if (event.xconfigure.width != app_.width() ||
                        event.xconfigure.height != app_.height()) {
                        app_.resize(event.xconfigure.width, event.xconfigure.height);
                        recreate_image(event.xconfigure.width, event.xconfigure.height);
                    }
                    break;
                case MotionNotify:
                    app_.on_motion(event.xmotion.x, event.xmotion.y);
                    break;
                case ButtonPress:
                    app_.on_button_press(map_button(event.xbutton.button),
                                         event.xbutton.x, event.xbutton.y);
                    break;
                case ButtonRelease:
                    app_.on_button_release(map_button(event.xbutton.button),
                                           event.xbutton.x, event.xbutton.y);
                    break;
                case KeyPress:
                    app_.on_key_press(map_key(XLookupKeysym(&event.xkey, 0)));
                    break;
                case KeyRelease:
                    app_.on_key_release(map_key(XLookupKeysym(&event.xkey, 0)));
                    break;
                case ClientMessage:
                    if (static_cast<Atom>(event.xclient.data.l[0]) == delete_atom_) {
                        app_.request_quit();
                    }
                    break;
                default:
                    break;
            }
        }
    }

    void destroy_image() {
        if (!image_) return;
        std::free(image_->data);
        image_->data = nullptr;
        XDestroyImage(image_);
        image_ = nullptr;
    }

    void rebuild_colour_tables() {
        for (int i = 0; i < 256; ++i) {
            red_lut_[static_cast<std::size_t>(i)] =
                static_cast<std::uint32_t>(pack_channel(static_cast<std::uint8_t>(i), image_->red_mask));
            green_lut_[static_cast<std::size_t>(i)] =
                static_cast<std::uint32_t>(pack_channel(static_cast<std::uint8_t>(i), image_->green_mask));
            blue_lut_[static_cast<std::size_t>(i)] =
                static_cast<std::uint32_t>(pack_channel(static_cast<std::uint8_t>(i), image_->blue_mask));
        }
        const std::uint16_t endian_test = 1;
        const bool host_lsb = *reinterpret_cast<const std::uint8_t*>(&endian_test) == 1;
        direct_32_ = image_->bits_per_pixel == 32 &&
            ((host_lsb && image_->byte_order == LSBFirst) ||
             (!host_lsb && image_->byte_order == MSBFirst));
    }

    void recreate_image(int width, int height) {
        destroy_image();
        image_ = XCreateImage(display_, visual_, DefaultDepth(display_, screen_),
                              ZPixmap, 0, nullptr,
                              static_cast<unsigned>(width), static_cast<unsigned>(height),
                              32, 0);
        if (!image_) throw std::runtime_error("Unable to create X11 presentation image");
        image_->data = static_cast<char*>(std::calloc(
            static_cast<std::size_t>(image_->bytes_per_line) * static_cast<std::size_t>(height), 1));
        if (!image_->data) throw std::bad_alloc();
        rebuild_colour_tables();
    }

    void present() {
        const InfiltratrSurface& surface = app_.surface();
        const int width = static_cast<int>(surface.width);
        const int height = static_cast<int>(surface.height);
        if (!surface.pixels || width <= 0 || height <= 0) return;

        if (direct_32_) {
            for (int y = 0; y < height; ++y) {
                auto* destination = reinterpret_cast<std::uint32_t*>(
                    image_->data + static_cast<std::size_t>(y) * image_->bytes_per_line);
                const std::uint32_t* source = surface.pixels + static_cast<std::size_t>(y) * surface.width;
                for (int x = 0; x < width; ++x) {
                    const std::uint32_t pixel = source[x];
                    const std::uint8_t r = static_cast<std::uint8_t>((pixel >> 16U) & 0xffU);
                    const std::uint8_t g = static_cast<std::uint8_t>((pixel >> 8U) & 0xffU);
                    const std::uint8_t b = static_cast<std::uint8_t>(pixel & 0xffU);
                    destination[x] = red_lut_[r] | green_lut_[g] | blue_lut_[b];
                }
            }
        } else {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const std::uint32_t pixel = surface.pixels[
                        static_cast<std::size_t>(y) * surface.width + static_cast<std::size_t>(x)];
                    const std::uint8_t r = static_cast<std::uint8_t>((pixel >> 16U) & 0xffU);
                    const std::uint8_t g = static_cast<std::uint8_t>((pixel >> 8U) & 0xffU);
                    const std::uint8_t b = static_cast<std::uint8_t>(pixel & 0xffU);
                    XPutPixel(image_, x, y, red_lut_[r] | green_lut_[g] | blue_lut_[b]);
                }
            }
        }
        XPutImage(display_, window_, gc_, image_, 0, 0, 0, 0,
                  static_cast<unsigned>(width), static_cast<unsigned>(height));
        XFlush(display_);
    }
};

} // namespace

int run(PixelApp& app) {
    X11Host host(app);
    return host.run();
}

} // namespace backyard_racer::platform
