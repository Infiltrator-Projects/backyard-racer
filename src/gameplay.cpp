// SPDX-License-Identifier: GPL-3.0-or-later
//
// Gameplay structure is deliberately informed by the GPL-2.0-or-later
// StreetRod3Classic project (garage/newspaper/parts/player/race separation),
// while this implementation is a fresh modern C++ port for Backyard Racer.

#include "gameplay.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace backyard_racer {

namespace {

constexpr int kStartingCash = 4000;

const std::vector<CarSpec> kOpponentCars = {
    {"opp_nova", 1966, "CHEVROLET", "NOVA", 2600, 220, 3000, 0.91, 4},
    {"opp_cougar", 1968, "MERCURY", "COUGAR", 3400, 260, 3400, 0.93, 4},
    {"opp_roadrunner", 1969, "PLYMOUTH", "ROAD RUNNER", 4300, 335, 3650, 0.95, 4},
    {"opp_boss", 1970, "FORD", "MUSTANG BOSS", 5200, 375, 3500, 0.97, 4},
};

const std::vector<std::string> kOpponentNames = {
    "EDDIE", "MICK", "RAY", "THE KING"
};

} // namespace

int OwnedCar::horsepower() const {
    int hp = base.horsepower;
    for (const auto& part : installed_parts) hp += part.horsepower_gain;
    return std::max(1, hp);
}

double OwnedCar::traction() const {
    double value = base.traction;
    for (const auto& part : installed_parts) value += part.traction_gain;
    return std::clamp(value, 0.70, 1.15);
}

double OwnedCar::shift_factor() const {
    double value = 1.0;
    for (const auto& part : installed_parts) value += part.shift_gain;
    return std::clamp(value, 0.85, 1.10);
}

int OwnedCar::resale_value() const {
    int value = base.price * 3 / 4;
    for (const auto& part : installed_parts) value += part.price / 2;
    return value;
}

std::string car_display_name(const CarSpec& car) {
    std::ostringstream out;
    out << car.year << ' ' << car.make << ' ' << car.model;
    return out.str();
}

std::string part_type_name(PartType type) {
    switch (type) {
        case PartType::Carburetor: return "CARBURETOR";
        case PartType::Intake: return "INTAKE";
        case PartType::Exhaust: return "EXHAUST";
        case PartType::Transmission: return "TRANSMISSION";
        case PartType::Tires: return "TIRES";
        case PartType::Camshaft: return "CAMSHAFT";
    }
    return "PART";
}

void GameState::seed_catalogs() {
    classifieds_ = {
        {"falcon64", 1964, "FORD", "FALCON", 1200, 150, 2700, 0.88, 3},
        {"mustang65", 1965, "FORD", "MUSTANG", 1800, 200, 2900, 0.90, 4},
        {"nova66", 1966, "CHEVROLET", "NOVA", 2100, 220, 3000, 0.91, 4},
        {"camaro67", 1967, "CHEVROLET", "CAMARO", 2800, 275, 3250, 0.93, 4},
        {"charger68", 1968, "DODGE", "CHARGER", 3300, 325, 3700, 0.94, 4},
        {"gto69", 1969, "PONTIAC", "GTO", 3600, 350, 3600, 0.95, 4},
        {"roadrunner69", 1969, "PLYMOUTH", "ROAD RUNNER", 3800, 335, 3650, 0.95, 4},
        {"challenger70", 1970, "DODGE", "CHALLENGER", 4200, 375, 3750, 0.96, 4},
    };

    parts_catalog_ = {
        {"carb_4bbl", "4 BARREL CARB", PartType::Carburetor, 280, 18, 0.00, 0.00},
        {"carb_dual", "DUAL QUAD CARBS", PartType::Carburetor, 520, 34, 0.00, 0.00},
        {"intake_hi", "HIGH RISE INTAKE", PartType::Intake, 340, 22, 0.00, 0.00},
        {"headers", "TUBE HEADERS", PartType::Exhaust, 320, 20, 0.00, 0.00},
        {"cam_street", "STREET CAM", PartType::Camshaft, 460, 32, 0.00, 0.00},
        {"cam_race", "RACE CAM", PartType::Camshaft, 720, 52, 0.00, 0.00},
        {"trans_close", "CLOSE RATIO 4 SPEED", PartType::Transmission, 620, 0, 0.00, -0.04},
        {"trans_quick", "QUICK SHIFT 4 SPEED", PartType::Transmission, 850, 0, 0.00, -0.07},
        {"tires_bias", "STICKY BIAS PLY", PartType::Tires, 380, 0, 0.07, 0.00},
        {"tires_drag", "DRAG SLICKS", PartType::Tires, 650, 0, 0.12, 0.00},
    };
}

void GameState::new_game() {
    started_ = true;
    cash_ = kStartingCash;
    garage_.clear();
    spare_parts_.clear();
    active_car_ = 0;
    opponent_index_ = 0;
    seed_catalogs();
}

const OwnedCar* GameState::active_car() const {
    if (garage_.empty() || active_car_ >= garage_.size()) return nullptr;
    return &garage_[active_car_];
}

OwnedCar* GameState::active_car() {
    if (garage_.empty() || active_car_ >= garage_.size()) return nullptr;
    return &garage_[active_car_];
}

void GameState::next_car() {
    if (garage_.empty()) return;
    active_car_ = (active_car_ + 1) % garage_.size();
}

bool GameState::buy_car(std::size_t listing_index, std::string* error) {
    if (!started_) {
        if (error) *error = "START A NEW GAME FIRST";
        return false;
    }
    if (listing_index >= classifieds_.size()) {
        if (error) *error = "THAT CAR IS NOT AVAILABLE";
        return false;
    }
    const CarSpec spec = classifieds_[listing_index];
    if (cash_ < spec.price) {
        if (error) *error = "NOT ENOUGH CASH";
        return false;
    }

    cash_ -= spec.price;
    garage_.push_back(OwnedCar{spec, {}});
    active_car_ = garage_.size() - 1;

    CarSpec replacement = spec;
    replacement.price = std::max(600, spec.price + 175);
    replacement.horsepower += 5;
    classifieds_[listing_index] = replacement;
    return true;
}

void GameState::install_part(OwnedCar& car, const PartSpec& part) {
    auto it = std::find_if(car.installed_parts.begin(), car.installed_parts.end(),
                           [&](const PartSpec& existing) { return existing.type == part.type; });
    if (it == car.installed_parts.end()) {
        car.installed_parts.push_back(part);
        return;
    }

    // Street Rod-style garage behaviour: the removed component is not destroyed.
    // It goes back into the player's parts bin and can be installed again later.
    spare_parts_.push_back(*it);
    *it = part;
}

bool GameState::buy_part(std::size_t part_index, std::string* error) {
    OwnedCar* car = active_car();
    if (!car) {
        if (error) *error = "BUY A CAR FIRST";
        return false;
    }
    if (part_index >= parts_catalog_.size()) {
        if (error) *error = "THAT PART IS NOT AVAILABLE";
        return false;
    }

    const PartSpec part = parts_catalog_[part_index];
    if (cash_ < part.price) {
        if (error) *error = "NOT ENOUGH CASH";
        return false;
    }

    cash_ -= part.price;
    install_part(*car, part);
    return true;
}

bool GameState::install_spare(std::size_t spare_index, std::string* error) {
    OwnedCar* car = active_car();
    if (!car) {
        if (error) *error = "BUY A CAR FIRST";
        return false;
    }
    if (spare_index >= spare_parts_.size()) {
        if (error) *error = "THAT SPARE PART IS NOT AVAILABLE";
        return false;
    }

    PartSpec part = spare_parts_[spare_index];
    spare_parts_.erase(spare_parts_.begin() + static_cast<std::ptrdiff_t>(spare_index));

    auto it = std::find_if(car->installed_parts.begin(), car->installed_parts.end(),
                           [&](const PartSpec& existing) { return existing.type == part.type; });
    if (it == car->installed_parts.end()) {
        car->installed_parts.push_back(part);
    } else {
        PartSpec removed = *it;
        *it = part;
        spare_parts_.push_back(removed);
    }
    return true;
}

Opponent GameState::current_opponent() const {
    const std::size_t index = opponent_index_ % kOpponentCars.size();
    Opponent opponent;
    opponent.name = kOpponentNames[index];
    opponent.car = OwnedCar{kOpponentCars[index], {}};
    opponent.reaction_seconds = 0.42 - static_cast<double>(index) * 0.055;
    return opponent;
}

double GameState::quarter_mile_et(const OwnedCar& car, double reaction_seconds) {
    const double hp = static_cast<double>(std::max(1, car.horsepower()));
    const double weight = static_cast<double>(std::max(1000, car.base.weight_lb));
    const double power_weight_et = 5.825 * std::cbrt(weight / hp);
    const double traction_penalty = std::max(0.0, 1.0 - car.traction()) * 2.2;
    const double shift_multiplier = car.shift_factor();
    return std::max(7.0, power_weight_et * shift_multiplier + traction_penalty + reaction_seconds);
}

void GameState::advance_opponent() {
    opponent_index_ = (opponent_index_ + 1) % kOpponentCars.size();
}

RaceResult GameState::race_for_cash(int wager) {
    RaceResult result;
    OwnedCar* player = active_car();
    if (!player) {
        result.summary = "BUY A CAR BEFORE YOU RACE";
        return result;
    }
    if (wager <= 0 || cash_ < wager) {
        result.summary = "YOU CANNOT COVER THAT BET";
        return result;
    }

    const Opponent opponent = current_opponent();
    result.valid = true;
    result.player_et = quarter_mile_et(*player, 0.30);
    result.opponent_et = quarter_mile_et(opponent.car, opponent.reaction_seconds);
    result.won = result.player_et <= result.opponent_et;
    result.cash_delta = result.won ? wager : -wager;
    cash_ += result.cash_delta;
    result.summary = result.won ? "YOU WON THE CASH RACE" : "YOU LOST THE CASH RACE";
    advance_opponent();
    return result;
}

RaceResult GameState::race_for_pink_slip() {
    RaceResult result;
    OwnedCar* player = active_car();
    if (!player) {
        result.summary = "BUY A CAR BEFORE YOU RACE";
        return result;
    }

    const Opponent opponent = current_opponent();
    result.valid = true;
    result.pink_slip = true;
    result.player_et = quarter_mile_et(*player, 0.30);
    result.opponent_et = quarter_mile_et(opponent.car, opponent.reaction_seconds);
    result.won = result.player_et <= result.opponent_et;

    if (result.won) {
        garage_.push_back(opponent.car);
        active_car_ = garage_.size() - 1;
        result.summary = "YOU WON HIS CAR";
    } else {
        garage_.erase(garage_.begin() + static_cast<std::ptrdiff_t>(active_car_));
        if (garage_.empty()) active_car_ = 0;
        else if (active_car_ >= garage_.size()) active_car_ = garage_.size() - 1;
        result.summary = "YOU LOST YOUR CAR";
    }

    advance_opponent();
    return result;
}

} // namespace backyard_racer
