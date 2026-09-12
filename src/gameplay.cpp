// SPDX-License-Identifier: GPL-3.0-or-later
//
// Gameplay structure is deliberately informed by the GPL-2.0-or-later
// StreetRod3Classic project (garage/newspaper/parts/player/race separation,
// reusable removed parts, damage repair costs and vehicle selling value),
// while this implementation is a fresh modern C++ port for Backyard Racer.

#include "gameplay.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace backyard_racer {

namespace {

constexpr int kStartingCash = 4000;
constexpr int kSaveVersion = 1;
constexpr std::size_t kMaxSavedCars = 64;
constexpr std::size_t kMaxSavedParts = 256;

const std::vector<CarSpec> kOpponentCars = {
    {"opp_nova", 1966, "CHEVROLET", "NOVA", 2600, 220, 3000, 0.91, 4},
    {"opp_cougar", 1968, "MERCURY", "COUGAR", 3400, 260, 3400, 0.93, 4},
    {"opp_roadrunner", 1969, "PLYMOUTH", "ROAD RUNNER", 4300, 335, 3650, 0.95, 4},
    {"opp_boss", 1970, "FORD", "MUSTANG BOSS", 5200, 375, 3500, 0.97, 4},
};

const std::vector<std::string> kOpponentNames = {
    "EDDIE", "MICK", "RAY", "THE KING"
};

void set_error(std::string* error, const std::string& message) {
    if (error) *error = message;
}

void write_car(std::ostream& out, const CarSpec& car) {
    out << "CAR " << std::quoted(car.id) << ' ' << car.year << ' '
        << std::quoted(car.make) << ' ' << std::quoted(car.model) << ' '
        << car.price << ' ' << car.horsepower << ' ' << car.weight_lb << ' '
        << std::setprecision(17) << car.traction << ' ' << car.gears << '\n';
}

bool read_car(std::istream& in, CarSpec& car) {
    std::string marker;
    if (!(in >> marker) || marker != "CAR") return false;
    return static_cast<bool>(in >> std::quoted(car.id) >> car.year
                                >> std::quoted(car.make) >> std::quoted(car.model)
                                >> car.price >> car.horsepower >> car.weight_lb
                                >> car.traction >> car.gears);
}

void write_part(std::ostream& out, const PartSpec& part) {
    out << "PART " << std::quoted(part.id) << ' ' << std::quoted(part.name) << ' '
        << static_cast<int>(part.type) << ' ' << part.price << ' '
        << part.horsepower_gain << ' ' << std::setprecision(17)
        << part.traction_gain << ' ' << part.shift_gain << '\n';
}

bool read_part(std::istream& in, PartSpec& part) {
    std::string marker;
    int type = 0;
    if (!(in >> marker) || marker != "PART") return false;
    if (!(in >> std::quoted(part.id) >> std::quoted(part.name) >> type
             >> part.price >> part.horsepower_gain >> part.traction_gain
             >> part.shift_gain)) return false;
    if (type < static_cast<int>(PartType::Carburetor) ||
        type > static_cast<int>(PartType::Engine)) return false;
    part.type = static_cast<PartType>(type);
    return true;
}

bool sane_car(const CarSpec& car) {
    return !car.id.empty() && car.year >= 1900 && car.year <= 2100 &&
           car.price >= 0 && car.horsepower > 0 && car.weight_lb >= 1000 &&
           std::isfinite(car.traction) && car.traction > 0.0 &&
           car.gears >= 1 && car.gears <= 10;
}

bool sane_part(const PartSpec& part) {
    return !part.id.empty() && part.price >= 0 &&
           std::isfinite(part.traction_gain) && std::isfinite(part.shift_gain);
}

} // namespace

GameState::GameState() {
    seed_catalogs();
    load_from(default_save_path(), nullptr);
}

int OwnedCar::horsepower() const {
    int hp = base.horsepower;
    for (const auto& part : installed_parts) hp += part.horsepower_gain;
    const double condition_factor = 0.70 + 0.30 * (std::clamp(condition, 0, 100) / 100.0);
    return std::max(1, static_cast<int>(std::lround(hp * condition_factor)));
}

double OwnedCar::traction() const {
    double value = base.traction;
    for (const auto& part : installed_parts) value += part.traction_gain;
    const double condition_factor = 0.90 + 0.10 * (std::clamp(condition, 0, 100) / 100.0);
    return std::clamp(value * condition_factor, 0.65, 1.15);
}

double OwnedCar::shift_factor() const {
    double value = 1.0;
    for (const auto& part : installed_parts) value += part.shift_gain;
    return std::clamp(value, 0.85, 1.10);
}

int OwnedCar::resale_value() const {
    int value = base.price * 3 / 4;
    for (const auto& part : installed_parts) value += part.price / 2;
    const double condition_factor = 0.55 + 0.45 * (std::clamp(condition, 0, 100) / 100.0);
    return std::max(100, static_cast<int>(std::lround(value * condition_factor)));
}

int OwnedCar::repair_cost() const {
    const int damage = 100 - std::clamp(condition, 0, 100);
    if (damage <= 0) return 0;
    int value = base.price;
    for (const auto& part : installed_parts) value += part.price / 2;
    return std::max(25, value * damage / 100);
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
        case PartType::Engine: return "ENGINE";
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
        {"engine_street_v8", "BUILT STREET V8", PartType::Engine, 1100, 90, 0.00, 0.00},
        {"engine_race_v8", "RACE PREP V8", PartType::Engine, 1850, 155, 0.00, 0.00},
    };
}

std::string GameState::default_save_path() {
    if (const char* explicit_path = std::getenv("BACKYARD_RACER_SAVE")) {
        if (*explicit_path) return explicit_path;
    }
    if (const char* xdg = std::getenv("XDG_DATA_HOME")) {
        if (*xdg) return (std::filesystem::path(xdg) / "backyard-racer" / "save_v1.txt").string();
    }
    if (const char* home = std::getenv("HOME")) {
        if (*home) return (std::filesystem::path(home) / ".local" / "share" /
                           "backyard-racer" / "save_v1.txt").string();
    }
    return "backyard-racer-save-v1.txt";
}

void GameState::persist() const {
    if (!started_) return;
    save_to(default_save_path(), nullptr);
}

void GameState::new_game() {
    started_ = true;
    cash_ = kStartingCash;
    reputation_ = 0;
    wins_ = 0;
    losses_ = 0;
    garage_.clear();
    spare_parts_.clear();
    active_car_ = 0;
    seed_catalogs();
    persist();
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
    persist();
}

bool GameState::buy_car(std::size_t listing_index, std::string* error) {
    if (!started_) {
        set_error(error, "START A NEW GAME FIRST");
        return false;
    }
    if (listing_index >= classifieds_.size()) {
        set_error(error, "THAT CAR IS NOT AVAILABLE");
        return false;
    }
    const CarSpec spec = classifieds_[listing_index];
    if (cash_ < spec.price) {
        set_error(error, "NOT ENOUGH CASH");
        return false;
    }

    cash_ -= spec.price;
    garage_.push_back(OwnedCar{spec, {}, 100});
    active_car_ = garage_.size() - 1;

    CarSpec replacement = spec;
    replacement.price = std::max(600, spec.price + 175);
    replacement.horsepower += 5;
    classifieds_[listing_index] = replacement;
    persist();
    return true;
}

bool GameState::sell_active_car(int* sale_price, std::string* error) {
    OwnedCar* car = active_car();
    if (!car) {
        set_error(error, "NO CAR TO SELL");
        return false;
    }

    const int price = car->resale_value();
    cash_ += price;
    if (sale_price) *sale_price = price;
    garage_.erase(garage_.begin() + static_cast<std::ptrdiff_t>(active_car_));
    if (garage_.empty()) active_car_ = 0;
    else if (active_car_ >= garage_.size()) active_car_ = garage_.size() - 1;
    persist();
    return true;
}

bool GameState::repair_active_car(int* repair_price, std::string* error) {
    OwnedCar* car = active_car();
    if (!car) {
        set_error(error, "NO CAR TO REPAIR");
        return false;
    }
    const int price = car->repair_cost();
    if (price <= 0) {
        set_error(error, "CAR IS ALREADY 100 PERCENT");
        return false;
    }
    if (cash_ < price) {
        set_error(error, "NOT ENOUGH CASH TO REPAIR");
        return false;
    }

    cash_ -= price;
    car->condition = 100;
    if (repair_price) *repair_price = price;
    persist();
    return true;
}

void GameState::install_part(OwnedCar& car, const PartSpec& part) {
    auto it = std::find_if(car.installed_parts.begin(), car.installed_parts.end(),
                           [&](const PartSpec& existing) { return existing.type == part.type; });
    if (it == car.installed_parts.end()) {
        car.installed_parts.push_back(part);
        return;
    }

    spare_parts_.push_back(*it);
    *it = part;
}

bool GameState::buy_part(std::size_t part_index, std::string* error) {
    OwnedCar* car = active_car();
    if (!car) {
        set_error(error, "BUY A CAR FIRST");
        return false;
    }
    if (part_index >= parts_catalog_.size()) {
        set_error(error, "THAT PART IS NOT AVAILABLE");
        return false;
    }

    const PartSpec part = parts_catalog_[part_index];
    if (cash_ < part.price) {
        set_error(error, "NOT ENOUGH CASH");
        return false;
    }

    cash_ -= part.price;
    install_part(*car, part);
    persist();
    return true;
}

bool GameState::install_spare(std::size_t spare_index, std::string* error) {
    OwnedCar* car = active_car();
    if (!car) {
        set_error(error, "BUY A CAR FIRST");
        return false;
    }
    if (spare_index >= spare_parts_.size()) {
        set_error(error, "THAT SPARE PART IS NOT AVAILABLE");
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
    persist();
    return true;
}

Opponent GameState::current_opponent() const {
    const std::size_t index = std::min<std::size_t>(static_cast<std::size_t>(reputation_ / 3),
                                                     kOpponentCars.size() - 1);
    Opponent opponent;
    opponent.name = kOpponentNames[index];
    opponent.car = OwnedCar{kOpponentCars[index], {}, 100};
    opponent.reaction_seconds = 0.42 - static_cast<double>(index) * 0.055;
    return opponent;
}

double GameState::quarter_mile_et(const OwnedCar& car, double reaction_seconds) {
    const double hp = static_cast<double>(std::max(1, car.horsepower()));
    const double weight = static_cast<double>(std::max(1000, car.base.weight_lb));
    const double power_weight_et = 5.825 * std::cbrt(weight / hp);
    const double traction_penalty = std::max(0.0, 1.0 - car.traction()) * 2.2;
    return std::max(7.0, power_weight_et * car.shift_factor() + traction_penalty + reaction_seconds);
}

void GameState::apply_race_outcome(OwnedCar& car, RaceResult& result, int win_reputation) {
    result.wear = result.won ? 2 : 5;
    car.condition = std::max(25, car.condition - result.wear);
    if (result.won) {
        ++wins_;
        result.reputation_delta = win_reputation;
        reputation_ += win_reputation;
    } else {
        ++losses_;
        const int loss = reputation_ > 0 ? -1 : 0;
        result.reputation_delta = loss;
        reputation_ = std::max(0, reputation_ + loss);
    }
}

RaceResult GameState::settle_race(const Opponent& opponent, int wager, bool pink_slip,
                                  double player_et) {
    RaceResult result;
    OwnedCar* player = active_car();
    if (!player) {
        result.summary = "BUY A CAR BEFORE YOU RACE";
        return result;
    }
    if (!std::isfinite(player_et) || player_et <= 0.0) {
        result.summary = "INVALID PLAYER RACE TIME";
        return result;
    }
    if (!pink_slip && (wager <= 0 || cash_ < wager)) {
        result.summary = "YOU CANNOT COVER THAT BET";
        return result;
    }

    result.valid = true;
    result.pink_slip = pink_slip;
    result.player_et = player_et;
    result.opponent_et = quarter_mile_et(opponent.car, opponent.reaction_seconds);
    result.won = result.player_et <= result.opponent_et;

    if (!pink_slip) {
        result.cash_delta = result.won ? wager : -wager;
        cash_ += result.cash_delta;
        apply_race_outcome(*player, result, 1);
        result.summary = result.won ? "YOU WON THE CASH RACE" : "YOU LOST THE CASH RACE";
        persist();
        return result;
    }

    apply_race_outcome(*player, result, 3);
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
    persist();
    return result;
}

RaceResult GameState::race_for_cash(int wager) {
    const OwnedCar* player = active_car();
    const double player_et = player ? quarter_mile_et(*player, 0.30) : 0.0;
    return settle_race(current_opponent(), wager, false, player_et);
}

RaceResult GameState::race_for_pink_slip() {
    const OwnedCar* player = active_car();
    const double player_et = player ? quarter_mile_et(*player, 0.30) : 0.0;
    return settle_race(current_opponent(), 0, true, player_et);
}

bool GameState::save_to(const std::string& path, std::string* error) const {
    if (!started_) {
        set_error(error, "NO GAME TO SAVE");
        return false;
    }

    const std::filesystem::path final_path(path);
    std::error_code ec;
    if (final_path.has_parent_path()) {
        std::filesystem::create_directories(final_path.parent_path(), ec);
        if (ec) {
            set_error(error, "COULD NOT CREATE SAVE DIRECTORY: " + ec.message());
            return false;
        }
    }

    const std::filesystem::path temp_path = final_path.string() + ".tmp";
    std::ofstream out(temp_path, std::ios::trunc);
    if (!out) {
        set_error(error, "COULD NOT OPEN SAVE FILE");
        return false;
    }

    out << "BACKYARD_RACER_SAVE " << kSaveVersion << '\n';
    out << "STATE " << cash_ << ' ' << reputation_ << ' ' << wins_ << ' '
        << losses_ << ' ' << active_car_ << '\n';

    out << "CLASSIFIEDS " << classifieds_.size() << '\n';
    for (const auto& car : classifieds_) write_car(out, car);

    out << "GARAGE " << garage_.size() << '\n';
    for (const auto& owned : garage_) {
        out << "OWNED " << std::clamp(owned.condition, 0, 100) << ' '
            << owned.installed_parts.size() << '\n';
        write_car(out, owned.base);
        for (const auto& part : owned.installed_parts) write_part(out, part);
    }

    out << "SPARES " << spare_parts_.size() << '\n';
    for (const auto& part : spare_parts_) write_part(out, part);
    out << "END\n";
    out.flush();
    if (!out) {
        set_error(error, "COULD NOT WRITE SAVE FILE");
        out.close();
        std::filesystem::remove(temp_path, ec);
        return false;
    }
    out.close();

    std::filesystem::rename(temp_path, final_path, ec);
    if (ec) {
        ec.clear();
        std::filesystem::remove(final_path, ec);
        ec.clear();
        std::filesystem::rename(temp_path, final_path, ec);
    }
    if (ec) {
        set_error(error, "COULD NOT PUBLISH SAVE FILE: " + ec.message());
        std::filesystem::remove(temp_path, ec);
        return false;
    }
    return true;
}

bool GameState::load_from(const std::string& path, std::string* error) {
    std::ifstream in(path);
    if (!in) {
        set_error(error, "NO SAVE FILE");
        return false;
    }

    std::string marker;
    int version = 0;
    if (!(in >> marker >> version) || marker != "BACKYARD_RACER_SAVE" || version != kSaveVersion) {
        set_error(error, "UNSUPPORTED OR CORRUPT SAVE FILE");
        return false;
    }

    int cash = 0, reputation = 0, wins = 0, losses = 0;
    std::size_t active = 0;
    if (!(in >> marker >> cash >> reputation >> wins >> losses >> active) || marker != "STATE" ||
        cash < 0 || reputation < 0 || wins < 0 || losses < 0) {
        set_error(error, "CORRUPT SAVE STATE");
        return false;
    }

    std::size_t classified_count = 0;
    if (!(in >> marker >> classified_count) || marker != "CLASSIFIEDS" ||
        classified_count == 0 || classified_count > kMaxSavedCars) {
        set_error(error, "CORRUPT CLASSIFIEDS DATA");
        return false;
    }
    std::vector<CarSpec> classifieds;
    classifieds.reserve(classified_count);
    for (std::size_t i = 0; i < classified_count; ++i) {
        CarSpec car;
        if (!read_car(in, car) || !sane_car(car)) {
            set_error(error, "CORRUPT CLASSIFIED CAR DATA");
            return false;
        }
        classifieds.push_back(std::move(car));
    }

    std::size_t garage_count = 0;
    if (!(in >> marker >> garage_count) || marker != "GARAGE" || garage_count > kMaxSavedCars) {
        set_error(error, "CORRUPT GARAGE DATA");
        return false;
    }
    std::vector<OwnedCar> garage;
    garage.reserve(garage_count);
    for (std::size_t i = 0; i < garage_count; ++i) {
        int condition = 0;
        std::size_t installed_count = 0;
        if (!(in >> marker >> condition >> installed_count) || marker != "OWNED" ||
            condition < 0 || condition > 100 || installed_count > kMaxSavedParts) {
            set_error(error, "CORRUPT OWNED CAR DATA");
            return false;
        }
        OwnedCar owned;
        owned.condition = condition;
        if (!read_car(in, owned.base) || !sane_car(owned.base)) {
            set_error(error, "CORRUPT GARAGE CAR DATA");
            return false;
        }
        owned.installed_parts.reserve(installed_count);
        for (std::size_t part_index = 0; part_index < installed_count; ++part_index) {
            PartSpec part;
            if (!read_part(in, part) || !sane_part(part)) {
                set_error(error, "CORRUPT INSTALLED PART DATA");
                return false;
            }
            owned.installed_parts.push_back(std::move(part));
        }
        garage.push_back(std::move(owned));
    }

    std::size_t spare_count = 0;
    if (!(in >> marker >> spare_count) || marker != "SPARES" || spare_count > kMaxSavedParts) {
        set_error(error, "CORRUPT SPARES DATA");
        return false;
    }
    std::vector<PartSpec> spares;
    spares.reserve(spare_count);
    for (std::size_t i = 0; i < spare_count; ++i) {
        PartSpec part;
        if (!read_part(in, part) || !sane_part(part)) {
            set_error(error, "CORRUPT SPARE PART DATA");
            return false;
        }
        spares.push_back(std::move(part));
    }

    if (!(in >> marker) || marker != "END") {
        set_error(error, "SAVE FILE IS INCOMPLETE");
        return false;
    }
    if (!garage.empty() && active >= garage.size()) {
        set_error(error, "SAVE FILE HAS INVALID ACTIVE CAR");
        return false;
    }
    if (garage.empty()) active = 0;

    seed_catalogs();
    classifieds_ = std::move(classifieds);
    garage_ = std::move(garage);
    spare_parts_ = std::move(spares);
    active_car_ = active;
    cash_ = cash;
    reputation_ = reputation;
    wins_ = wins;
    losses_ = losses;
    started_ = true;
    return true;
}

} // namespace backyard_racer
