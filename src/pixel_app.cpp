// SPDX-License-Identifier: GPL-3.0-or-later
// Pixel renderer kept split while the architecture cutover lands.
#include "scene_embedded_assets.h"
namespace backyard_racer::scene_assets {
inline constexpr const IndexedAsset& garage_hd = garage;
#include "falcon_hd_asset.inc"
#include "mustang_hd_asset.inc"
}

#include "pixel_app_part1.inc"
#include "pixel_bilinear_scaling.inc"
#define scaled_copy scaled_copy_bilinear
#include "pixel_app_part2.inc"
#include "pixel_app_part3.inc"
#include "pixel_app_part4.inc"
#include "pixel_app_part5.inc"
#undef scaled_copy
