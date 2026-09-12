// SPDX-License-Identifier: GPL-3.0-or-later
#include "gameplay.h"
#include "race_session.h"

#include <infiltratr/timing.h>

#include <iostream>
#include <string>

using namespace backyard_racer;

namespace {
int fail(const std::string& message) {
    std::cerr << "race session test failed: " << message << '\n';
    return 1;
}
}

int main() {
    GameState game;
    game.new_game();

    std::string error;
    if (!game.buy_car(0, &error)) return fail("could not buy starter car: " + error);
    const OwnedCar* car = game.active_car();
    if (!car) return fail("starter car missing");

    const Opponent opponent = game.current_opponent();
    DragRaceSession race;
    race.start(*car, opponent);
    race.set_throttle(true);

    InfiltratrFixedStepScheduler scheduler{};
    if (!infiltratr_fixed_step_configure(&scheduler, 1000000000ULL, 60ULL,
                                         250000000ULL, 8ULL))
        return fail("Common fixed-step scheduler configuration failed");

    InfiltratrFixedStepResult timing{};
    if (!infiltratr_fixed_step_advance(&scheduler, 0ULL, &timing))
        return fail("Common fixed-step scheduler baseline failed");

    constexpr double step_seconds = 1.0 / 60.0;
    constexpr std::uint64_t sample_ns = 1000000ULL;
    constexpr std::uint64_t timeout_ns = 30000000000ULL;

    for (std::uint64_t now = sample_ns; now <= timeout_ns && !race.finished(); now += sample_ns) {
        if (!infiltratr_fixed_step_advance(&scheduler, now, &timing))
            return fail("Common fixed-step scheduler advance failed");
        if (timing.dropped_steps != 0 || timing.clamped_ticks != 0)
            return fail("normal race cadence unexpectedly dropped simulation time");

        for (std::uint64_t step = 0; step < timing.steps_to_run && !race.finished(); ++step) {
            if (race.green() && !race.shifting() && race.rpm() >= 6000)
                race.shift_up();
            race.update(step_seconds);
        }
    }

    if (!race.finished()) return fail("quarter mile did not finish within 30 seconds");
    if (race.player_et() <= 0.0 || race.distance_ft() < 1319.9)
        return fail("player quarter-mile result is invalid");
    if (race.opponent_et() <= 0.0)
        return fail("opponent elapsed time is invalid");
    if (race.gear() < 2)
        return fail("manual shifting path was not exercised");

    const RaceResult result = game.settle_race(opponent, 100, false, race.player_et());
    if (!result.valid) return fail("interactive race could not settle into game economy");
    if (result.player_et != race.player_et())
        return fail("interactive elapsed time was not preserved by settlement");
    if (game.wins() + game.losses() != 1)
        return fail("interactive race did not update the career record");

    std::cout << "race session test passed in " << race.player_et() << " seconds\n";
    return 0;
}
