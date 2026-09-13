// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "gameplay.h"

#include <string_view>
#include <vector>

namespace backyard_racer {

const std::vector<EngineSpec>& historical_engine_catalog();
const std::vector<CarSpec>& historical_vehicle_variants();

const EngineSpec* find_engine_spec(std::string_view id);
bool engine_family_fits_chassis(std::string_view chassis_family,
                                std::string_view engine_family);

} // namespace backyard_racer
