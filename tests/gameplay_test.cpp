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

    const int stock_hp = game.active_car()->horsepower();
    if (!game.buy_part(0, &error)) return fail("could not buy first carb: " + error);
    const int first_carb_hp = game.active_car()->horsepower();
    if (first_carb_hp <= stock_hp) return fail("first performance part did not increase horsepower");

    if (!game.buy_part(1, &error)) return fail("could not buy replacement carb: " + error);
    const int second_carb_hp = game.active_car()->horsepower();
    if (second_carb_hp <= first_carb_hp) return fail("replacement carb did not improve performance");
    if (game.spare_parts().size() != 1) return fail("replaced carb was not returned to parts bin");

    if (!game.install_spare(0, &error)) return fail("could not reinstall spare carb: " + error);
    if (game.active_car()->horsepower() != first_carb_hp) return fail("spare carb was not reinstalled");
    if (game.spare_parts().size() != 1) return fail("swapped-out carb was not returned to parts bin");

    const auto opponent = game.current_opponent();
    if (opponent.name.empty()) return fail("opponent was not generated");

    const auto result = game.race_for_cash(100);
    if (!result.valid) return fail("cash race was rejected");
    if (result.player_et <= 0.0 || result.opponent_et <= 0.0)
        return fail("race elapsed times were not generated");

    std::cout << "gameplay smoke test passed\n";
    return 0;
}
