// SPDX-License-Identifier: GPL-3.0-or-later
// Pixel renderer kept split while the architecture cutover lands.
#include "scene_embedded_assets.h"
#include "pixel_app.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace backyard_racer::scene_assets {
#include "garage_hd_asset.inc"
#include "falcon_hd_asset.inc"
#include "mustang_hd_asset.inc"
#include "ui_font_asset.inc"
#include "menu_chrome_asset.inc"
}

#include "pixel_app_part1.inc"
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

// The original all-US-units draw dispatcher remains available for migration,
// but the active draw() is supplied by pixel_au_screens.inc through the menu
// skin include at the end of PixelApp::Impl.
#define draw legacy_draw
#include "pixel_app_part4.inc"
#undef draw
#include "pixel_app_part5.inc"
