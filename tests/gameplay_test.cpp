// SPDX-License-Identifier: GPL-3.0-or-later
#include "gameplay.h"

#include <cassert>
#include <iostream>

using namespace backyard_racer;

int main() {
    GameState game;
    game.new_game();
    assert(game.started());
    assert(game.cash() == 4000);
    assert(!game.classifieds().empty());

    std::string error;
    assert(game.buy_car(0, &error));
    assert(game.active_car() != nullptr);
    assert(game.cash() == 2800);

    const int before_hp = game.active_car()->horsepower();
    assert(game.buy_part(0, &error));
    assert(game.active_car()->horsepower() > before_hp);

    const auto opponent = game.current_opponent();
    assert(!opponent.name.empty());

    const auto result = game.race_for_cash(100);
    assert(result.valid);
    assert(result.player_et > 0.0);
    assert(result.opponent_et > 0.0);

    std::cout << "gameplay smoke test passed\n";
    return 0;
}
