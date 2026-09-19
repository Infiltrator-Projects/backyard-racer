// SPDX-License-Identifier: GPL-3.0-or-later
#include "gameplay.h"
#include "pixel_app.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

std::uint64_t frame_hash(const InfiltratrSurface& surface) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(surface.pixels);
    const std::size_t size =
        surface.width * surface.height * sizeof(*surface.pixels);
    std::uint64_t hash = 1469598103934665603ULL;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace

int main() {
    const char* save_path = "/tmp/backyard-racer-pixel-core-test.save";
    std::remove(save_path);
    if (::setenv("BACKYARD_RACER_SAVE", save_path, 1) != 0) return 10;

    backyard_racer::PixelApp app(800, 600);
    app.draw();
    const InfiltratrSurface& surface = app.surface();
    if (!surface.pixels || surface.width != 800 || surface.height != 600) return 1;

    // New Game -> newspaper.
    app.on_button_press(backyard_racer::PixelMouseButton::Left, 100, 270);
    app.draw();
    const std::uint64_t listings_hash = frame_hash(app.surface());

    // First ad. A listing click must open the article and must not buy the car.
    app.on_button_press(backyard_racer::PixelMouseButton::Left, 160, 110);
    app.draw();
    if (frame_hash(app.surface()) == listings_hash) return 3;

    backyard_racer::GameState persisted;
    if (!persisted.started()) return 4;
    if (!persisted.garage().empty()) return 5;

    app.resize(960, 540);
    app.draw();
    if (!app.surface().pixels || app.surface().width != 960 || app.surface().height != 540) return 2;

    std::remove(save_path);
    return 0;
}
