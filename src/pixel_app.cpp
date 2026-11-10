// SPDX-License-Identifier: GPL-3.0-or-later
// Pixel renderer kept split while the architecture cutover lands.
#include "scene_embedded_assets.hpp"
#include "pixel_app.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace backyard_racer::scene_assets {
#include "garage_hd_asset.inc"
#include "vehicle_photo_assets.inc"
#include "ui_font_asset.inc"
#include "menu_chrome_asset.inc"
}

#include "pixel_app_part1.inc"
#include "pixel_app_part2.inc"

// Legacy renderers have explicit names in their owning .inc files.
// The active menu and Australian screens retain their original names.
#include "pixel_app_part3.inc"
#include "pixel_app_part4.inc"
#include "pixel_app_part5.inc"
