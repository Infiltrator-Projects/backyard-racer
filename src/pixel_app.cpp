// SPDX-License-Identifier: GPL-3.0-or-later
// Pixel renderer kept split while the architecture cutover lands.
#include "scene_embedded_assets.h"
#include "pixel_app.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

static std::uint8_t backyard_mix_channel(unsigned a00, unsigned a10,
                                         unsigned a01, unsigned a11,
                                         double fx, double fy) {
    const double top = a00 + (static_cast<double>(a10) - a00) * fx;
    const double bottom = a01 + (static_cast<double>(a11) - a01) * fx;
    return static_cast<std::uint8_t>(
        std::clamp(std::lround(top + (bottom - top) * fy), 0L, 255L));
}

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
            infiltratr_surface_set_pixel(destination, destination_x + dx, destination_y + dy,
                InfiltratrColor{
                    backyard_mix_channel(c00.r,c10.r,c01.r,c11.r,fx,fy),
                    backyard_mix_channel(c00.g,c10.g,c01.g,c11.g,fx,fy),
                    backyard_mix_channel(c00.b,c10.b,c01.b,c11.b,fx,fy),
                    backyard_mix_channel(c00.a,c10.a,c01.a,c11.a,fx,fy)});
        }
    }
}

static void backyard_surface_blit_region_scaled_smooth(InfiltratrSurface* destination,
                                                        const InfiltratrSurface* source,
                                                        int source_x, int source_y,
                                                        int source_width, int source_height,
                                                        int destination_x, int destination_y,
                                                        int destination_width, int destination_height) {
    if (!destination || !source || !destination->pixels || !source->pixels ||
        source_width <= 0 || source_height <= 0 ||
        destination_width <= 0 || destination_height <= 0) return;

    for (int dy = 0; dy < destination_height; ++dy) {
        const double syf = destination_height == 1 ? 0.0 :
            static_cast<double>(dy) * (source_height - 1) / (destination_height - 1);
        const int sy0 = source_y + static_cast<int>(syf);
        const int sy1 = std::min(source_y + source_height - 1, sy0 + 1);
        const double fy = syf - static_cast<int>(syf);
        for (int dx = 0; dx < destination_width; ++dx) {
            const double sxf = destination_width == 1 ? 0.0 :
                static_cast<double>(dx) * (source_width - 1) / (destination_width - 1);
            const int sx0 = source_x + static_cast<int>(sxf);
            const int sx1 = std::min(source_x + source_width - 1, sx0 + 1);
            const double fx = sxf - static_cast<int>(sxf);

            const InfiltratrColor c00 = infiltratr_surface_get_pixel(source, sx0, sy0);
            const InfiltratrColor c10 = infiltratr_surface_get_pixel(source, sx1, sy0);
            const InfiltratrColor c01 = infiltratr_surface_get_pixel(source, sx0, sy1);
            const InfiltratrColor c11 = infiltratr_surface_get_pixel(source, sx1, sy1);
            const InfiltratrColor src{
                backyard_mix_channel(c00.r,c10.r,c01.r,c11.r,fx,fy),
                backyard_mix_channel(c00.g,c10.g,c01.g,c11.g,fx,fy),
                backyard_mix_channel(c00.b,c10.b,c01.b,c11.b,fx,fy),
                backyard_mix_channel(c00.a,c10.a,c01.a,c11.a,fx,fy)};
            if (src.a == 0) continue;

            const int tx = destination_x + dx;
            const int ty = destination_y + dy;
            if (tx < 0 || ty < 0 ||
                tx >= static_cast<int>(destination->width) ||
                ty >= static_cast<int>(destination->height)) continue;
            if (src.a == 255) {
                infiltratr_surface_set_pixel(destination, tx, ty, src);
                continue;
            }
            const InfiltratrColor dst = infiltratr_surface_get_pixel(destination, tx, ty);
            const unsigned a = src.a;
            const unsigned ia = 255U - a;
            infiltratr_surface_set_pixel(destination, tx, ty, InfiltratrColor{
                static_cast<std::uint8_t>((src.r * a + dst.r * ia + 127U) / 255U),
                static_cast<std::uint8_t>((src.g * a + dst.g * ia + 127U) / 255U),
                static_cast<std::uint8_t>((src.b * a + dst.b * ia + 127U) / 255U),
                255});
        }
    }
}

namespace backyard_racer::scene_assets {
#include "garage_hd_asset.inc"
#include "falcon_hd_asset.inc"
#include "mustang_hd_asset.inc"
#include "ui_font_asset.inc"
#include "menu_chrome_asset.inc"
}

#define infiltratr_surface_blit_scaled_nearest backyard_surface_blit_scaled_smooth
#include "pixel_app_part1.inc"
#undef infiltratr_surface_blit_scaled_nearest
#include "pixel_app_part2.inc"

// Keep the old primitive menu definitions under private legacy names while
// the active member functions come from pixel_menu_skin.inc at the class end.
#define menu_new legacy_menu_new
#define menu_continue legacy_menu_continue
#define menu_settings legacy_menu_settings
#define menu_quit legacy_menu_quit
#define menu_button legacy_menu_button
#define draw_menu legacy_draw_menu
#include "pixel_app_part3.inc"
#undef draw_menu
#undef menu_button
#undef menu_quit
#undef menu_settings
#undef menu_continue
#undef menu_new

#include "pixel_app_part4.inc"
#include "pixel_app_part5.inc"
