// SPDX-License-Identifier: GPL-3.0-or-later
// Pixel renderer kept split while the architecture cutover lands.
#include "scene_embedded_assets.h"
#include "pixel_app.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

static void backyard_surface_blit_scaled_smooth(InfiltratrSurface* destination,
                                                 const InfiltratrSurface* source,
                                                 int destination_x, int destination_y,
                                                 int destination_width, int destination_height) {
    if (!destination || !source || !destination->pixels || !source->pixels ||
        destination_width <= 0 || destination_height <= 0 ||
        source->width == 0 || source->height == 0) return;

    const int sw = static_cast<int>(source->width);
    const int sh = static_cast<int>(source->height);
    for (int dy = 0; dy < destination_height; ++dy) {
        const double sy = destination_height == 1 ? 0.0 :
            static_cast<double>(dy) * (sh - 1) / (destination_height - 1);
        const int y0 = static_cast<int>(sy);
        const int y1 = std::min(y0 + 1, sh - 1);
        const double fy = sy - y0;
        for (int dx = 0; dx < destination_width; ++dx) {
            const double sx = destination_width == 1 ? 0.0 :
                static_cast<double>(dx) * (sw - 1) / (destination_width - 1);
            const int x0 = static_cast<int>(sx);
            const int x1 = std::min(x0 + 1, sw - 1);
            const double fx = sx - x0;
            const InfiltratrColor c00 = infiltratr_surface_get_pixel(source, x0, y0);
            const InfiltratrColor c10 = infiltratr_surface_get_pixel(source, x1, y0);
            const InfiltratrColor c01 = infiltratr_surface_get_pixel(source, x0, y1);
            const InfiltratrColor c11 = infiltratr_surface_get_pixel(source, x1, y1);
            auto mix = [fx, fy](unsigned a00, unsigned a10, unsigned a01, unsigned a11) {
                const double top = a00 + (static_cast<double>(a10) - a00) * fx;
                const double bottom = a01 + (static_cast<double>(a11) - a01) * fx;
                return static_cast<std::uint8_t>(std::clamp(std::lround(top + (bottom - top) * fy), 0L, 255L));
            };
            infiltratr_surface_set_pixel(destination, destination_x + dx, destination_y + dy,
                InfiltratrColor{mix(c00.r,c10.r,c01.r,c11.r),
                                mix(c00.g,c10.g,c01.g,c11.g),
                                mix(c00.b,c10.b,c01.b,c11.b),
                                mix(c00.a,c10.a,c01.a,c11.a)});
        }
    }
}

namespace backyard_racer::scene_assets {
#include "garage_hd_asset.inc"
#include "falcon_hd_asset.inc"
#include "mustang_hd_asset.inc"
}

#define infiltratr_surface_blit_scaled_nearest backyard_surface_blit_scaled_smooth
#include "pixel_app_part1.inc"
#undef infiltratr_surface_blit_scaled_nearest
#include "pixel_app_part2.inc"
#include "pixel_app_part3.inc"
#include "pixel_app_part4.inc"
#include "pixel_app_part5.inc"
