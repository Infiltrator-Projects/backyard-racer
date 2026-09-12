// SPDX-License-Identifier: GPL-3.0-or-later
#include "race_session.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace backyard_racer {

namespace {
constexpr double kQuarterMileFeet = 1320.0;
constexpr std::array<double, 5> kRpmPerMph = {0.0, 120.0, 80.0, 55.0, 40.0};
constexpr std::array<double, 5> kGearForce = {0.0, 1.50, 1.15, 0.88, 0.68};
}

void DragRaceSession::start(const OwnedCar& car, const Opponent& opponent) {
    active_ = true;
    green_ = false;
    finished_ = false;
    throttle_ = false;
    reacted_ = false;
    countdown_ = 2.5;
    elapsed_ = 0.0;
    player_et_ = 0.0;
    distance_ft_ = 0.0;
    opponent_distance_ft_ = 0.0;
    speed_mph_ = 0.0;
    reaction_seconds_ = 0.0;
    shift_cut_ = 0.0;
    launch_rpm_ = 1000.0;
    rpm_ = 1000;
    gear_ = 1;
    max_gears_ = std::clamp(car.base.gears, 1, 4);
    horsepower_ = std::max(1, car.horsepower());
    weight_lb_ = std::max(1000, car.base.weight_lb);
    traction_ = car.traction();
    shift_factor_ = car.shift_factor();
    opponent_et_ = GameState::quarter_mile_et(opponent.car, opponent.reaction_seconds);
}

void DragRaceSession::shift_up() {
    if (!active_ || finished_ || gear_ >= max_gears_) return;
    ++gear_;
    // StreetRod3 drops acceleration during a shift. Faster transmissions shorten
    // the interruption through the existing shift factor.
    shift_cut_ = 0.24 * shift_factor_;
    update_rpm();
}

void DragRaceSession::shift_down() {
    if (!active_ || finished_ || gear_ <= 1) return;
    --gear_;
    shift_cut_ = 0.18 * shift_factor_;
    update_rpm();
}

void DragRaceSession::update_rpm() {
    if (!green_ && speed_mph_ < 0.1) {
        rpm_ = static_cast<int>(std::clamp(launch_rpm_, 900.0, 6500.0));
        return;
    }
    const int index = std::clamp(gear_, 1, 4);
    rpm_ = static_cast<int>(std::clamp(1000.0 + speed_mph_ * kRpmPerMph[index], 900.0, 7600.0));
}

void DragRaceSession::update(double dt_seconds) {
    if (!active_ || finished_) return;
    const double dt = std::clamp(dt_seconds, 0.0, 0.05);

    if (!green_) {
        // Allow the player to stage/rev while waiting for green, but do not move.
        const double target = throttle_ ? 4800.0 : 1000.0;
        launch_rpm_ += (target - launch_rpm_) * std::min(1.0, dt * 4.5);
        update_rpm();
        countdown_ -= dt;
        if (countdown_ <= 0.0) {
            countdown_ = 0.0;
            green_ = true;
            if (throttle_) {
                reacted_ = true;
                reaction_seconds_ = 0.10;
            }
        }
        return;
    }

    elapsed_ += dt;
    if (!reacted_ && throttle_) {
        reacted_ = true;
        reaction_seconds_ = elapsed_;
    }

    if (shift_cut_ > 0.0) shift_cut_ = std::max(0.0, shift_cut_ - dt);
    update_rpm();

    const int index = std::clamp(gear_, 1, 4);
    const double powerband = std::max(0.30, 1.0 - std::abs(static_cast<double>(rpm_) - 5200.0) / 5600.0);
    const double throttle = (throttle_ && shift_cut_ <= 0.0) ? 1.0 : 0.0;
    const double engine_accel = 14.0 * (horsepower_ / 200.0) * (3000.0 / weight_lb_) *
                                traction_ * kGearForce[index] * powerband * throttle;
    const double aero_drag = 0.0007 * speed_mph_ * speed_mph_;
    speed_mph_ = std::max(0.0, speed_mph_ + (engine_accel - aero_drag) * dt);
    distance_ft_ += speed_mph_ * 1.4666666667 * dt;

    // Give the opponent an acceleration-shaped progress curve while preserving
    // the deterministic ET produced by the existing gameplay model.
    const double opponent_fraction = opponent_et_ > 0.0
        ? std::clamp(std::pow(elapsed_ / opponent_et_, 1.32), 0.0, 1.0)
        : 0.0;
    opponent_distance_ft_ = kQuarterMileFeet * opponent_fraction;

    update_rpm();
    if (distance_ft_ >= kQuarterMileFeet) {
        distance_ft_ = kQuarterMileFeet;
        player_et_ = elapsed_;
        finished_ = true;
        active_ = false;
    }
}

} // namespace backyard_racer
