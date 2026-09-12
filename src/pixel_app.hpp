// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <infiltratr/graphics.h>

#include <memory>

namespace backyard_racer {

enum class PixelKey {
    Unknown,
    Escape,
    Up,
    Down,
    Left,
    Right,
    Space,
    Enter,
    W,
    A,
    S,
    D
};

enum class PixelMouseButton {
    Unknown,
    Left,
    Middle,
    Right,
    WheelUp,
    WheelDown
};

class PixelApp {
public:
    PixelApp(int width, int height);
    ~PixelApp();

    PixelApp(const PixelApp&) = delete;
    PixelApp& operator=(const PixelApp&) = delete;

    bool running() const;
    bool dirty() const;
    void rendered();

    int width() const;
    int height() const;
    const InfiltratrSurface& surface() const;

    void resize(int width, int height);
    void tick(double dt_seconds);
    void draw();

    void on_motion(int x, int y);
    void on_button_press(PixelMouseButton button, int x, int y);
    void on_button_release(PixelMouseButton button, int x, int y);
    void on_key_press(PixelKey key);
    void on_key_release(PixelKey key);
    void request_quit();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace backyard_racer
