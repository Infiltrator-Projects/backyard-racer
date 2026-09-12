// SPDX-License-Identifier: GPL-3.0-or-later
#include "gameplay.h"

#include <iostream>
#include <string>

using namespace backyard_racer;

namespace {
int fail(const std::string& message) {
    std::cerr << "gameplay smoke test failed: " << message << '\n';
    return 1;
}
}

int main() {
    GameState game;
    game.new_game();

    if (!game.started()) return fail("new game did not start");
    if (game.cash() != 4000) return fail("starting cash is wrong");
    if (game.classifieds().empty()) return fail("classifieds are empty");

    std::string error;
    if (!game.buy_car(0, &error)) return fail("could not buy first car: " + error);
    if (game.active_car() == nullptr) return fail("bought car was not added to garage");
    if (game.cash() != 2800) return fail("car purchase did not debit cash correctly");

    const int before_hp = game.active_car()->horsepower();
    if (!game.buy_part(0, &error)) return fail("could not buy first part: " + error);
    if (game.active_car() == nullptr || game.active_car()->horsepower() <= before_hp)
        return fail("performance part did not increase horsepower");

    const auto opponent = game.current_opponent();
    if (opponent.name.empty()) return fail("opponent was not generated");

    const auto result = game.race_for_cash(100);
    if (!result.valid) return fail("cash race was rejected");
    if (result.player_et <= 0.0 || result.opponent_et <= 0.0)
        return fail("race elapsed times were not generated");

    std::cout << "gameplay smoke test passed\n";
    return 0;
}
