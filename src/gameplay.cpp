// SPDX-License-Identifier: GPL-3.0-or-later
//
// Gameplay structure is deliberately informed by the GPL-2.0-or-later
// StreetRod3Classic project (garage/newspaper/parts/player/race separation,
// reusable removed parts, damage repair costs and vehicle selling value),
// while this implementation is a fresh modern C++ port for Backyard Racer.

#include "gameplay.h"
#include "vehicle_data.h"

#include <infiltratr/posix.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <system_error>

namespace backyard_racer {

namespace {

constexpr int kStartingCash = 4000;
constexpr int kSaveVersion = 3;
constexpr std::size_t kMaxSavedCars = 64;
constexpr std::size_t kMaxSavedParts = 256;
constexpr std::size_t kWeeklyListingCount = 8;

struct FactoryPaint {
    const char* name;
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

const std::vector<CarSpec> kOpponentCars = {
    {"nova66", 1966, "CHEVROLET", "NOVA", 2600, 275, 2800, 0.93, 4,
     "opp_nova_327", "327 4V V8", "chevy_nova66", "chevy_327_4v_275", 20, false},
    {"cougar68", 1968, "MERCURY", "COUGAR GT", 3900, 325, 3440, 0.95, 4,
     "opp_cougar_390", "390 GT FE V8", "ford_cougar68", "ford_390_fe_325", 20, false},
    {"roadrunner69", 1969, "PLYMOUTH", "ROAD RUNNER", 4300, 335, 3450, 0.95, 4,
     "opp_roadrunner_383", "383 HP V8", "mopar_roadrunner69", "mopar_383_hp_335", 20, false},
    {"opp_boss", 1970, "FORD", "MUSTANG BOSS", 5200, 375, 3500, 0.97, 4},
};

const std::vector<std::string> kOpponentNames = {
    "EDDIE", "MICK", "RAY", "THE KING"
};

void set_error(std::string* error, const std::string& message) {
    if (error) *error = message;
}

PaintState paint_from(const FactoryPaint& paint) {
    return PaintState{paint.r, paint.g, paint.b, paint.name, false};
}

PaintState choose_factory_paint(const CarSpec& car) {
    static const std::array<FactoryPaint, 5> falcon64{{
        {"WIMBLEDON WHITE", 238, 235, 218},
        {"RANGOON RED", 174, 55, 47},
        {"GUARDSMAN BLUE", 53, 76, 111},
        {"DYNASTY GREEN", 73, 101, 82},
        {"PRAIRIE BRONZE", 157, 128, 91},
    }};
    static const std::array<FactoryPaint, 6> mustang65{{
        {"WIMBLEDON WHITE", 238, 235, 218},
        {"RANGOON RED", 174, 55, 47},
        {"CASPIAN BLUE", 47, 72, 103},
        {"IVY GREEN", 62, 89, 66},
        {"POPPY RED", 207, 72, 38},
        {"SILVER BLUE", 112, 139, 153},
    }};
    static const std::array<FactoryPaint, 6> generic60s{{
        {"FACTORY WHITE", 238, 236, 224},
        {"FACTORY RED", 171, 48, 43},
        {"FACTORY BLUE", 54, 80, 112},
        {"FACTORY GREEN", 62, 91, 70},
        {"FACTORY BLACK", 34, 34, 32},
        {"FACTORY GOLD", 171, 143, 91},
    }};

    std::random_device rd;
    std::mt19937 generator(rd());
    if (car.id == "falcon64") {
        std::uniform_int_distribution<std::size_t> pick(0, falcon64.size() - 1);
        return paint_from(falcon64[pick(generator)]);
    }
    if (car.id == "mustang65") {
        std::uniform_int_distribution<std::size_t> pick(0, mustang65.size() - 1);
        return paint_from(mustang65[pick(generator)]);
    }
    std::uniform_int_distribution<std::size_t> pick(0, generic60s.size() - 1);
    return paint_from(generic60s[pick(generator)]);
}

std::uint32_t fresh_market_seed() {
    std::random_device rd;
    std::uint32_t seed = rd();
    seed ^= (rd() << 1U) | (rd() >> 31U);
    if (seed == 0) seed = 0xB4C4A4D1U;
    return seed;
}

std::uint32_t market_week_seed(std::uint32_t seed, int week) {
    std::uint32_t value = seed ^ (0x9E3779B9U * static_cast<std::uint32_t>(week));
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    value *= 0x846CA68BU;
    value ^= value >> 16U;
    return value;
}

void write_car(std::ostream& out, const CarSpec& car) {
    out << "CAR " << std::quoted(car.id) << ' ' << car.year << ' '
        << std::quoted(car.make) << ' ' << std::quoted(car.model) << ' '
        << car.price << ' ' << car.horsepower << ' ' << car.weight_lb << ' '
        << std::setprecision(17) << car.traction << ' ' << car.gears << ' '
        << std::quoted(car.variant_id) << ' ' << std::quoted(car.variant) << ' '
        << std::quoted(car.chassis_family) << ' ' << std::quoted(car.engine_id) << ' '
        << car.rarity_weight << ' ' << (car.starter_ok ? 1 : 0) << '\n';
}

bool read_car(std::istream& in, CarSpec& car, int version) {
    std::string marker;
    if (!(in >> marker) || marker != "CAR") return false;
    if (!(in >> std::quoted(car.id) >> car.year
             >> std::quoted(car.make) >> std::quoted(car.model)
             >> car.price >> car.horsepower >> car.weight_lb
             >> car.traction >> car.gears)) return false;
    if (version >= 3) {
        int starter = 0;
        if (!(in >> std::quoted(car.variant_id) >> std::quoted(car.variant)
                 >> std::quoted(car.chassis_family) >> std::quoted(car.engine_id)
                 >> car.rarity_weight >> starter)) return false;
        if (starter != 0 && starter != 1) return false;
        car.starter_ok = starter != 0;
    }
    return true;
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
           car.gears >= 1 && car.gears <= 10 && car.rarity_weight >= 0;
}

bool sane_part(const PartSpec& part) {
    return !part.id.empty() && part.price >= 0 &&
           std::isfinite(part.traction_gain) && std::isfinite(part.shift_gain);
}

void enrich_legacy_car(CarSpec& car) {
    if (!car.chassis_family.empty() && !car.engine_id.empty()) return;
    const auto& variants = historical_vehicle_variants();
    const CarSpec* best = nullptr;
    int best_delta = std::numeric_limits<int>::max();
    for (const auto& candidate : variants) {
        if (candidate.id != car.id) continue;
        const int delta = std::abs(candidate.horsepower - car.horsepower);
        if (delta < best_delta) {
            best = &candidate;
            best_delta = delta;
        }
    }
    if (!best) return;
    car.variant_id = best->variant_id;
    car.variant = best->variant;
    car.chassis_family = best->chassis_family;
    car.engine_id = best->engine_id;
    car.rarity_weight = best->rarity_weight;
    car.starter_ok = best->starter_ok;
}

PartSpec engine_spare_part(const EngineSpec& engine) {
    return PartSpec{"engine:" + engine.id, "ENGINE - " + engine.name,
                    PartType::Engine, engine.price, 0, 0.0, 0.0};
}

const EngineSpec* engine_from_part(const PartSpec& part) {
    constexpr std::string_view prefix = "engine:";
    if (part.type != PartType::Engine || part.id.rfind(prefix, 0) != 0) return nullptr;
    return find_engine_spec(std::string_view(part.id).substr(prefix.size()));
}

void apply_engine(OwnedCar& car, const EngineSpec& next) {
    const EngineSpec* previous = find_engine_spec(car.base.engine_id);
    if (previous) {
        car.base.weight_lb = std::max(1000, car.base.weight_lb + next.weight_lb - previous->weight_lb);
    }
    car.base.engine_id = next.id;
    car.base.horsepower = next.horsepower;
    car.base.variant = next.name;
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
    vehicle_catalog_ = historical_vehicle_variants();
    engine_catalog_ = historical_engine_catalog();
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

void GameState::generate_classifieds() {
    classifieds_.clear();
    if (vehicle_catalog_.empty()) return;

    std::mt19937 generator(market_week_seed(market_seed_, current_week_));
    std::vector<std::size_t> available(vehicle_catalog_.size());
    for (std::size_t i = 0; i < available.size(); ++i) available[i] = i;

    auto body_count = [&](const std::string& body_id) {
        return static_cast<int>(std::count_if(classifieds_.begin(), classifieds_.end(),
            [&](const CarSpec& car) { return car.id == body_id; }));
    };

    auto choose = [&](bool starter_only, bool enforce_body_limit) -> std::size_t {
        std::uint64_t total = 0;
        for (const std::size_t index : available) {
            const CarSpec& candidate = vehicle_catalog_[index];
            if (starter_only && !candidate.starter_ok) continue;
            if (enforce_body_limit && body_count(candidate.id) >= 2) continue;
            total += static_cast<std::uint64_t>(std::max(1, candidate.rarity_weight));
        }
        if (total == 0) return vehicle_catalog_.size();
        std::uniform_int_distribution<std::uint64_t> draw(1, total);
        std::uint64_t needle = draw(generator);
        for (const std::size_t index : available) {
            const CarSpec& candidate = vehicle_catalog_[index];
            if (starter_only && !candidate.starter_ok) continue;
            if (enforce_body_limit && body_count(candidate.id) >= 2) continue;
            const std::uint64_t weight = static_cast<std::uint64_t>(std::max(1, candidate.rarity_weight));
            if (needle <= weight) return index;
            needle -= weight;
        }
        return vehicle_catalog_.size();
    };

    auto add_listing = [&](std::size_t catalog_index) {
        auto it = std::find(available.begin(), available.end(), catalog_index);
        if (it == available.end()) return;
        CarSpec listing = vehicle_catalog_[catalog_index];
        std::uniform_int_distribution<int> price_adjust(-10, 12);
        const int percent = 100 + price_adjust(generator);
        listing.price = std::max(500, (listing.price * percent + 50) / 100);
        classifieds_.push_back(std::move(listing));
        available.erase(it);
    };

    // Every paper carries at least two realistic starter choices. They are
    // variants, so a cheap I6 and a V8 of the same body can both appear.
    for (int i = 0; i < 2 && classifieds_.size() < kWeeklyListingCount; ++i) {
        const std::size_t index = choose(true, true);
        if (index == vehicle_catalog_.size()) break;
        add_listing(index);
    }

    while (!available.empty() && classifieds_.size() < kWeeklyListingCount) {
        std::size_t index = choose(false, true);
        if (index == vehicle_catalog_.size()) index = choose(false, false);
        if (index == vehicle_catalog_.size()) break;
        add_listing(index);
    }
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
    current_week_ = 1;
    market_seed_ = fresh_market_seed();
    garage_.clear();
    spare_parts_.clear();
    active_car_ = 0;
    seed_catalogs();
    generate_classifieds();
    persist();
}

void GameState::advance_week() {
    if (!started_) return;
    ++current_week_;
    generate_classifieds();
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
    garage_.push_back(OwnedCar{spec, {}, 100, choose_factory_paint(spec)});
    active_car_ = garage_.size() - 1;
    classifieds_.erase(classifieds_.begin() + static_cast<std::ptrdiff_t>(listing_index));
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

bool GameState::repaint_active_car(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                   const std::string& name, bool custom,
                                   std::string* error) {
    OwnedCar* car = active_car();
    if (!car) {
        set_error(error, "NO CAR TO PAINT");
        return false;
    }
    car->paint = PaintState{r, g, b, name.empty() ? std::string("CUSTOM") : name, custom};
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

std::vector<EngineSpec> GameState::compatible_engines() const {
    std::vector<EngineSpec> result;
    const OwnedCar* car = active_car();
    if (!car || car->base.chassis_family.empty()) return result;
    for (const auto& engine : engine_catalog_) {
        if (engine.id == car->base.engine_id) continue;
        if (engine_family_fits_chassis(car->base.chassis_family, engine.family)) {
            result.push_back(engine);
        }
    }
    std::stable_sort(result.begin(), result.end(), [](const EngineSpec& a, const EngineSpec& b) {
        if (a.price != b.price) return a.price < b.price;
        return a.horsepower < b.horsepower;
    });
    return result;
}

bool GameState::swap_engine(const std::string& engine_id, int* installed_price,
                            std::string* error) {
    OwnedCar* car = active_car();
    if (!car) {
        set_error(error, "BUY A CAR FIRST");
        return false;
    }
    const EngineSpec* next = find_engine_spec(engine_id);
    if (!next) {
        set_error(error, "UNKNOWN ENGINE");
        return false;
    }
    if (car->base.engine_id == next->id) {
        set_error(error, "THAT ENGINE IS ALREADY INSTALLED");
        return false;
    }
    if (!engine_family_fits_chassis(car->base.chassis_family, next->family)) {
        set_error(error, "ENGINE DOES NOT FIT THIS CHASSIS");
        return false;
    }
    if (cash_ < next->price) {
        set_error(error, "NOT ENOUGH CASH FOR THAT ENGINE");
        return false;
    }

    if (const EngineSpec* previous = find_engine_spec(car->base.engine_id)) {
        spare_parts_.push_back(engine_spare_part(*previous));
    }
    // Retire any pre-v3 generic engine upgrade when a real engine is fitted.
    auto legacy = std::find_if(car->installed_parts.begin(), car->installed_parts.end(),
        [](const PartSpec& part) { return part.type == PartType::Engine; });
    if (legacy != car->installed_parts.end()) {
        spare_parts_.push_back(*legacy);
        car->installed_parts.erase(legacy);
    }

    cash_ -= next->price;
    apply_engine(*car, *next);
    if (installed_price) *installed_price = next->price;
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
    if (const EngineSpec* next = engine_from_part(part)) {
        if (!engine_family_fits_chassis(car->base.chassis_family, next->family)) {
            set_error(error, "SPARE ENGINE DOES NOT FIT THIS CHASSIS");
            return false;
        }
        if (car->base.engine_id == next->id) {
            set_error(error, "THAT ENGINE IS ALREADY INSTALLED");
            return false;
        }
        PartSpec old_engine;
        bool have_old = false;
        if (const EngineSpec* previous = find_engine_spec(car->base.engine_id)) {
            old_engine = engine_spare_part(*previous);
            have_old = true;
        }
        spare_parts_.erase(spare_parts_.begin() + static_cast<std::ptrdiff_t>(spare_index));
        apply_engine(*car, *next);
        if (have_old) spare_parts_.push_back(std::move(old_engine));
        persist();
        return true;
    }

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
    opponent.car = OwnedCar{kOpponentCars[index], {}, 100, choose_factory_paint(kOpponentCars[index])};
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

    /*
     * Build one complete save image in memory, then hand publication to
     * Common's durable atomic-file contract. A successful return means the
     * replacement and its parent-directory entry have both been synced.
     */
    std::ostringstream out;
    out << "BACKYARD_RACER_SAVE " << kSaveVersion << '\n';
    out << "STATE " << cash_ << ' ' << reputation_ << ' ' << wins_ << ' '
        << losses_ << ' ' << active_car_ << ' ' << current_week_ << ' '
        << market_seed_ << '\n';

    out << "CLASSIFIEDS " << classifieds_.size() << '\n';
    for (const auto& car : classifieds_) write_car(out, car);

    out << "GARAGE " << garage_.size() << '\n';
    for (const auto& owned : garage_) {
        out << "OWNED " << std::clamp(owned.condition, 0, 100) << ' '
            << owned.installed_parts.size() << ' '
            << static_cast<unsigned>(owned.paint.r) << ' '
            << static_cast<unsigned>(owned.paint.g) << ' '
            << static_cast<unsigned>(owned.paint.b) << ' '
            << std::quoted(owned.paint.name) << ' '
            << (owned.paint.custom ? 1 : 0) << '\n';
        write_car(out, owned.base);
        for (const auto& part : owned.installed_parts) write_part(out, part);
    }

    out << "SPARES " << spare_parts_.size() << '\n';
    for (const auto& part : spare_parts_) write_part(out, part);
    out << "END\n";

    if (!out) {
        set_error(error, "COULD NOT SERIALIZE SAVE FILE");
        return false;
    }
    const std::string payload = out.str();
    const std::string native_path = final_path.string();
    const int write_error = infiltratr_atomic_file_write_bytes(
        native_path.c_str(), INFILTRATR_ATOMIC_FILE_PRIVATE,
        payload.data(), payload.size());
    if (write_error != 0) {
        set_error(error, "COULD NOT PUBLISH SAVE FILE: " +
                         std::error_code(write_error, std::generic_category()).message());
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
    if (!(in >> marker >> version) || marker != "BACKYARD_RACER_SAVE" ||
        version < 1 || version > kSaveVersion) {
        set_error(error, "UNSUPPORTED OR CORRUPT SAVE FILE");
        return false;
    }

    int cash = 0, reputation = 0, wins = 0, losses = 0;
    std::size_t active = 0;
    int week = 1;
    std::uint32_t market_seed = 0;
    if (!(in >> marker >> cash >> reputation >> wins >> losses >> active) || marker != "STATE" ||
        cash < 0 || reputation < 0 || wins < 0 || losses < 0) {
        set_error(error, "CORRUPT SAVE STATE");
        return false;
    }
    if (version >= 3) {
        if (!(in >> week >> market_seed) || week < 1 || market_seed == 0) {
            set_error(error, "CORRUPT WEEKLY MARKET STATE");
            return false;
        }
    } else {
        market_seed = 0xB4C4A4D1U ^ static_cast<std::uint32_t>(cash) ^
                      (static_cast<std::uint32_t>(reputation) << 16U);
    }

    std::size_t classified_count = 0;
    if (!(in >> marker >> classified_count) || marker != "CLASSIFIEDS" ||
        classified_count > kMaxSavedCars) {
        set_error(error, "CORRUPT CLASSIFIEDS DATA");
        return false;
    }
    std::vector<CarSpec> classifieds;
    classifieds.reserve(classified_count);
    for (std::size_t i = 0; i < classified_count; ++i) {
        CarSpec car;
        if (!read_car(in, car, version) || !sane_car(car)) {
            set_error(error, "CORRUPT CLASSIFIED CAR DATA");
            return false;
        }
        if (version < 3) enrich_legacy_car(car);
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
        PaintState paint;
        if (!(in >> marker >> condition >> installed_count) || marker != "OWNED" ||
            condition < 0 || condition > 100 || installed_count > kMaxSavedParts) {
            set_error(error, "CORRUPT OWNED CAR DATA");
            return false;
        }
        if (version >= 2) {
            unsigned r = 0, g = 0, b = 0;
            int custom = 0;
            if (!(in >> r >> g >> b >> std::quoted(paint.name) >> custom) ||
                r > 255 || g > 255 || b > 255 || (custom != 0 && custom != 1)) {
                set_error(error, "CORRUPT PAINT DATA");
                return false;
            }
            paint.r = static_cast<std::uint8_t>(r);
            paint.g = static_cast<std::uint8_t>(g);
            paint.b = static_cast<std::uint8_t>(b);
            paint.custom = custom != 0;
        }
        OwnedCar owned;
        owned.condition = condition;
        if (!read_car(in, owned.base, version) || !sane_car(owned.base)) {
            set_error(error, "CORRUPT GARAGE CAR DATA");
            return false;
        }
        if (version < 3) enrich_legacy_car(owned.base);
        if (version == 1) paint = choose_factory_paint(owned.base);
        owned.paint = std::move(paint);
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
    current_week_ = week;
    market_seed_ = market_seed;
    started_ = true;
    return true;
}

} // namespace backyard_racer
