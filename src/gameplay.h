// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace backyard_racer {

enum class PartType {
    Carburetor,
    Intake,
    Exhaust,
    Transmission,
    Tires,
    Camshaft
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

struct CarSpec {
    std::string id;
    int year = 0;
    std::string make;
    std::string model;
    int price = 0;
    int horsepower = 0;
    int weight_lb = 0;
    double traction = 1.0;
    int gears = 4;
};

struct OwnedCar {
    CarSpec base;
    std::vector<PartSpec> installed_parts;

    int horsepower() const;
    double traction() const;
    double shift_factor() const;
    int resale_value() const;
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
    std::string summary;
};

class GameState {
public:
    void new_game();

    bool started() const { return started_; }
    int cash() const { return cash_; }
    const std::vector<CarSpec>& classifieds() const { return classifieds_; }
    const std::vector<PartSpec>& parts_catalog() const { return parts_catalog_; }
    const std::vector<PartSpec>& spare_parts() const { return spare_parts_; }
    const std::vector<OwnedCar>& garage() const { return garage_; }

    std::size_t active_car_index() const { return active_car_; }
    const OwnedCar* active_car() const;
    OwnedCar* active_car();
    void next_car();

    bool buy_car(std::size_t listing_index, std::string* error = nullptr);
    bool buy_part(std::size_t part_index, std::string* error = nullptr);
    bool install_spare(std::size_t spare_index, std::string* error = nullptr);

    Opponent current_opponent() const;
    RaceResult race_for_cash(int wager);
    RaceResult race_for_pink_slip();

    static double quarter_mile_et(const OwnedCar& car, double reaction_seconds);

private:
    bool started_ = false;
    int cash_ = 0;
    std::vector<CarSpec> classifieds_;
    std::vector<PartSpec> parts_catalog_;
    std::vector<PartSpec> spare_parts_;
    std::vector<OwnedCar> garage_;
    std::size_t active_car_ = 0;
    std::size_t opponent_index_ = 0;

    void seed_catalogs();
    void advance_opponent();
    void install_part(OwnedCar& car, const PartSpec& part);
};

std::string car_display_name(const CarSpec& car);
std::string part_type_name(PartType type);

} // namespace backyard_racer
