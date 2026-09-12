// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixel_app.hpp"
#include "scene_embedded_assets.h"

#include <cstdint>
#include <iostream>
#include <string_view>

namespace {
std::uint64_t fnv1a(std::string_view value) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : value) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}
}

int main() {
    std::cerr << std::hex
              << "garage " << backyard_racer::scene_assets::garage.zlib_b64.size() << ' '
              << fnv1a(backyard_racer::scene_assets::garage.zlib_b64) << '\n'
              << "falcon " << backyard_racer::scene_assets::falcon.zlib_b64.size() << ' '
              << fnv1a(backyard_racer::scene_assets::falcon.zlib_b64) << '\n'
              << "mustang " << backyard_racer::scene_assets::mustang.zlib_b64.size() << ' '
              << fnv1a(backyard_racer::scene_assets::mustang.zlib_b64) << std::dec << '\n';

    backyard_racer::PixelApp app(800, 600);
    app.draw();
    const InfiltratrSurface& surface = app.surface();
    if (!surface.pixels || surface.width != 800 || surface.height != 600) return 1;
    app.resize(960, 540);
    app.draw();
    if (!app.surface().pixels || app.surface().width != 960 || app.surface().height != 540) return 2;
    return 0;
}
