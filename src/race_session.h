// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "gameplay.h"

namespace backyard_racer {

// Lightweight interactive quarter-mile session. The control flow is informed by
// StreetRod3Classic's red-light/racing states, manual gear inputs and clutch cut,
// but the simulation is a fresh Backyard Racer implementation.
class DragRaceSession {
public:
    void start(const OwnedCar& car, const Opponent& opponent);
    void update(double dt_seconds);

    void set_throttle(bool pressed) { throttle_ = pressed; }
    void shift_up();
    void shift_down();

    bool active() const { return active_; }
    bool green() const { return green_; }
    bool finished() const { return finished_; }
    bool throttle() const { return throttle_; }
    bool shifting() const { return shift_cut_ > 0.0; }

    double countdown() const { return countdown_; }
    double elapsed() const { return elapsed_; }
    double player_et() const { return player_et_; }
    double opponent_et() const { return opponent_et_; }
    double distance_ft() const { return distance_ft_; }
    double opponent_distance_ft() const { return opponent_distance_ft_; }
    double speed_mph() const { return speed_mph_; }
    double reaction_seconds() const { return reaction_seconds_; }
    int rpm() const { return rpm_; }
    int gear() const { return gear_; }
    int max_gears() const { return max_gears_; }

private:
    bool active_ = false;
    bool green_ = false;
    bool finished_ = false;
    bool throttle_ = false;
    bool reacted_ = false;

    double countdown_ = 2.5;
    double elapsed_ = 0.0;
    double player_et_ = 0.0;
    double opponent_et_ = 0.0;
    double distance_ft_ = 0.0;
    double opponent_distance_ft_ = 0.0;
    double speed_mph_ = 0.0;
    double reaction_seconds_ = 0.0;
    double shift_cut_ = 0.0;
    double launch_rpm_ = 1000.0;

    int rpm_ = 1000;
    int gear_ = 1;
    int max_gears_ = 4;
    int horsepower_ = 150;
    int weight_lb_ = 2800;
    double traction_ = 0.9;
    double shift_factor_ = 1.0;

    void update_rpm();
};

} // namespace backyard_racer
