// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixel_app.hpp"

int main() {
    backyard_racer::PixelApp app(800, 600);
    app.draw();
    const InfiltratrSurface& surface = app.surface();
    if (!surface.pixels || surface.width != 800 || surface.height != 600) return 1;
    app.resize(960, 540);
    app.draw();
    if (!app.surface().pixels || app.surface().width != 960 || app.surface().height != 540) return 2;
    return 0;
}
