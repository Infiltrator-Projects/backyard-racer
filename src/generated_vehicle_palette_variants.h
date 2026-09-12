// SPDX-License-Identifier: CC0-1.0
// Palette derivatives of Kenney's CC0 Isometric Tiles Vehicles sedan.
// The upstream pack ships civilian cars in Black, Blue, Green, Red and Silver;
// these compact X11 variants reuse the imported red sedan geometry so the
// classifieds can present a period-looking mixed used-car lot without adding
// a PNG decoder to the native renderer.
#pragma once

#include "generated_isometric_assets.h"

namespace backyard_racer::assets {

inline constexpr SpriteRange kenney_iso_green_sedan_ranges[] = {
    {0, 4, {57, 57, 57}}, {4, 27, {88, 83, 84}}, {31, 6, {45, 76, 52}},
    {37, 8, {117, 157, 189}}, {45, 4, {123, 165, 199}}, {49, 5, {61, 104, 69}},
    {54, 6, {141, 167, 194}}, {60, 37, {156, 154, 170}}, {97, 27, {78, 132, 87}},
    {124, 15, {112, 154, 118}}, {139, 27, {91, 151, 101}}
};
inline constexpr EmbeddedSprite kenney_iso_green_sedan = {
    32, 29, kenney_iso_red_sedan_rects,
    sizeof(kenney_iso_red_sedan_rects) / sizeof(kenney_iso_red_sedan_rects[0]),
    kenney_iso_green_sedan_ranges,
    sizeof(kenney_iso_green_sedan_ranges) / sizeof(kenney_iso_green_sedan_ranges[0])
};
inline constexpr EmbeddedSprite kenney_iso_green_sedan_garage = {
    48, 29, kenney_iso_red_sedan_rects,
    sizeof(kenney_iso_red_sedan_rects) / sizeof(kenney_iso_red_sedan_rects[0]),
    kenney_iso_green_sedan_ranges,
    sizeof(kenney_iso_green_sedan_ranges) / sizeof(kenney_iso_green_sedan_ranges[0])
};

inline constexpr SpriteRange kenney_iso_silver_sedan_ranges[] = {
    {0, 4, {49, 51, 54}}, {4, 27, {78, 81, 85}}, {31, 6, {73, 78, 83}},
    {37, 8, {117, 157, 189}}, {45, 4, {123, 165, 199}}, {49, 5, {105, 111, 118}},
    {54, 6, {141, 167, 194}}, {60, 37, {151, 153, 160}}, {97, 27, {139, 146, 153}},
    {124, 15, {174, 179, 184}}, {139, 27, {192, 196, 200}}
};
inline constexpr EmbeddedSprite kenney_iso_silver_sedan = {
    32, 29, kenney_iso_red_sedan_rects,
    sizeof(kenney_iso_red_sedan_rects) / sizeof(kenney_iso_red_sedan_rects[0]),
    kenney_iso_silver_sedan_ranges,
    sizeof(kenney_iso_silver_sedan_ranges) / sizeof(kenney_iso_silver_sedan_ranges[0])
};
inline constexpr EmbeddedSprite kenney_iso_silver_sedan_garage = {
    48, 29, kenney_iso_red_sedan_rects,
    sizeof(kenney_iso_red_sedan_rects) / sizeof(kenney_iso_red_sedan_rects[0]),
    kenney_iso_silver_sedan_ranges,
    sizeof(kenney_iso_silver_sedan_ranges) / sizeof(kenney_iso_silver_sedan_ranges[0])
};

inline constexpr SpriteRange kenney_iso_black_sedan_ranges[] = {
    {0, 4, {40, 41, 43}}, {4, 27, {61, 62, 65}}, {31, 6, {18, 20, 23}},
    {37, 8, {102, 137, 165}}, {45, 4, {111, 149, 180}}, {49, 5, {33, 36, 40}},
    {54, 6, {127, 151, 175}}, {60, 37, {124, 126, 132}}, {97, 27, {51, 55, 60}},
    {124, 15, {82, 87, 93}}, {139, 27, {67, 72, 78}}
};
inline constexpr EmbeddedSprite kenney_iso_black_sedan = {
    32, 29, kenney_iso_red_sedan_rects,
    sizeof(kenney_iso_red_sedan_rects) / sizeof(kenney_iso_red_sedan_rects[0]),
    kenney_iso_black_sedan_ranges,
    sizeof(kenney_iso_black_sedan_ranges) / sizeof(kenney_iso_black_sedan_ranges[0])
};
inline constexpr EmbeddedSprite kenney_iso_black_sedan_garage = {
    48, 29, kenney_iso_red_sedan_rects,
    sizeof(kenney_iso_red_sedan_rects) / sizeof(kenney_iso_red_sedan_rects[0]),
    kenney_iso_black_sedan_ranges,
    sizeof(kenney_iso_black_sedan_ranges) / sizeof(kenney_iso_black_sedan_ranges[0])
};

} // namespace backyard_racer::assets
