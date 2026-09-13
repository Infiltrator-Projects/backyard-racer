// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace backyard_racer {

enum class PartType {
    Carburetor,
    Intake,
    Exhaust,
    Transmission,
    Tires,
    Camshaft,
    Engine
};

struct PartSpec {
    std::string id;
    std::string name;
    PartType type{};
    int price = 0;
    int horsepower_gain = 0;
    double traction_gain = 0.0;
    double shift_gain = 0.0;
};

struct EngineSpec {
    std::string id;
    std::string manufacturer;
    std::string family;
    std::string name;
    std::string layout;
    int displacement_ci = 0;
    int horsepower = 0;
    int weight_lb = 0;
    int price = 0;
};

struct CarSpec {
    // id is the body/art identity. Multiple drivetrain variants intentionally
    // share it, e.g. every 1965 Mustang uses the mustang65 body artwork.
    std::string id;
    int year = 0;
    std::string make;
    std::string model;
    int price = 0;
    int horsepower = 0;
    int weight_lb = 0;
    double traction = 1.0;
    int gears = 4;

    // Variant/fitment metadata introduced with the weekly used-car market.
    std::string variant_id;
    std::string variant;
    std::string chassis_family;
    std::string engine_id;
    int rarity_weight = 100;
    bool starter_ok = false;
};

struct PaintState {
    std::uint8_t r = 192;
    std::uint8_t g = 192;
    std::uint8_t b = 192;
    std::string name = "FACTORY GREY";
    bool custom = false;
};

struct OwnedCar {
    CarSpec base;
    std::vector<PartSpec> installed_parts;
    int condition = 100;
    PaintState paint{};

    int horsepower() const;
    double traction() const;
    double shift_factor() const;
    int resale_value() const;
    int repair_cost() const;
};

struct Opponent {
    std::string name;
    OwnedCar car;
    double reaction_seconds = 0.35;
};

struct RaceResult {
    bool valid = false;
    bool won = false;
    bool pink_slip = false;
    double player_et = 0.0;
    double opponent_et = 0.0;
    int cash_delta = 0;
    int reputation_delta = 0;
    int wear = 0;
    std::string summary;
};

class GameState {
public:
    GameState();

    void new_game();

    bool started() const { return started_; }
    int cash() const { return cash_; }
    int reputation() const { return reputation_; }
    int wins() const { return wins_; }
    int losses() const { return losses_; }
    int current_week() const { return current_week_; }
    const std::vector<CarSpec>& classifieds() const { return classifieds_; }
    const std::vector<CarSpec>& vehicle_catalog() const { return vehicle_catalog_; }
    const std::vector<EngineSpec>& engine_catalog() const { return engine_catalog_; }
    const std::vector<PartSpec>& parts_catalog() const { return parts_catalog_; }
    const std::vector<PartSpec>& spare_parts() const { return spare_parts_; }
    const std::vector<OwnedCar>& garage() const { return garage_; }

    std::size_t active_car_index() const { return active_car_; }
    const OwnedCar* active_car() const;
    OwnedCar* active_car();
    void next_car();

    // Time only advances when the game explicitly says so. This keeps a week's
    // newspaper stable across visits/reloads, then regenerates it atomically.
    void advance_week();

    bool buy_car(std::size_t listing_index, std::string* error = nullptr);
    bool sell_active_car(int* sale_price = nullptr, std::string* error = nullptr);
    bool repair_active_car(int* repair_price = nullptr, std::string* error = nullptr);
    bool repaint_active_car(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                            const std::string& name = "CUSTOM",
                            bool custom = true,
                            std::string* error = nullptr);
    bool buy_part(std::size_t part_index, std::string* error = nullptr);
    bool install_spare(std::size_t spare_index, std::string* error = nullptr);

    std::vector<EngineSpec> compatible_engines() const;
    bool swap_engine(const std::string& engine_id, int* installed_price = nullptr,
                     std::string* error = nullptr);

    Opponent current_opponent() const;
    RaceResult settle_race(const Opponent& opponent, int wager, bool pink_slip,
                           double player_et);
    RaceResult race_for_cash(int wager);
    RaceResult race_for_pink_slip();

    bool save_to(const std::string& path, std::string* error = nullptr) const;
    bool load_from(const std::string& path, std::string* error = nullptr);
    static std::string default_save_path();

    static double quarter_mile_et(const OwnedCar& car, double reaction_seconds);

private:
    bool started_ = false;
    int cash_ = 0;
    int reputation_ = 0;
    int wins_ = 0;
    int losses_ = 0;
    int current_week_ = 1;
    std::uint32_t market_seed_ = 0;
    std::vector<CarSpec> vehicle_catalog_;
    std::vector<EngineSpec> engine_catalog_;
    std::vector<CarSpec> classifieds_;
    std::vector<PartSpec> parts_catalog_;
    std::vector<PartSpec> spare_parts_;
    std::vector<OwnedCar> garage_;
    std::size_t active_car_ = 0;

    void seed_catalogs();
    void generate_classifieds();
    void install_part(OwnedCar& car, const PartSpec& part);
    void apply_race_outcome(OwnedCar& car, RaceResult& result, int win_reputation);
    void persist() const;
};

std::string car_display_name(const CarSpec& car);
std::string part_type_name(PartType type);

} // namespace backyard_racer
