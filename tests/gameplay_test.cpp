// SPDX-License-Identifier: GPL-3.0-or-later
#include "gameplay.h"

#include <filesystem>
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
    if (game.reputation() != 0 || game.wins() != 0 || game.losses() != 0)
        return fail("new game record was not reset");
    if (game.classifieds().empty()) return fail("classifieds are empty");

    std::string error;
    if (!game.buy_car(0, &error)) return fail("could not buy first car: " + error);
    if (game.active_car() == nullptr) return fail("bought car was not added to garage");
    if (game.cash() != 2800) return fail("car purchase did not debit cash correctly");
    if (game.active_car()->condition != 100) return fail("newly bought car did not start at full condition");

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

    std::size_t engine_index = game.parts_catalog().size();
    for (std::size_t i = 0; i < game.parts_catalog().size(); ++i) {
        if (game.parts_catalog()[i].type == PartType::Engine) {
            engine_index = i;
            break;
        }
    }
    if (engine_index == game.parts_catalog().size()) return fail("engine upgrades are missing from the catalog");
    const int hp_before_engine = game.active_car()->horsepower();
    if (!game.buy_part(engine_index, &error)) return fail("could not buy engine upgrade: " + error);
    if (game.active_car()->horsepower() <= hp_before_engine)
        return fail("engine upgrade did not increase horsepower");

    const auto save_path = std::filesystem::temp_directory_path() / "backyard-racer-gameplay-test.sav";
    std::error_code remove_error;
    std::filesystem::remove(save_path, remove_error);
    if (!game.save_to(save_path.string(), &error)) return fail("could not save game: " + error);

    GameState restored;
    if (!restored.load_from(save_path.string(), &error)) return fail("could not reload game: " + error);
    std::filesystem::remove(save_path, remove_error);
    if (!restored.started()) return fail("reloaded game was not marked started");
    if (restored.cash() != game.cash()) return fail("cash did not survive save/load");
    if (restored.garage().size() != game.garage().size()) return fail("garage did not survive save/load");
    if (restored.spare_parts().size() != game.spare_parts().size()) return fail("parts bin did not survive save/load");
    if (!restored.active_car() || restored.active_car()->base.id != game.active_car()->base.id)
        return fail("active car did not survive save/load");
    if (restored.active_car()->horsepower() != game.active_car()->horsepower())
        return fail("installed performance parts did not survive save/load");

    const auto opponent = game.current_opponent();
    if (opponent.name.empty()) return fail("opponent was not generated");

    const auto result = game.race_for_cash(100);
    if (!result.valid) return fail("cash race was rejected");
    if (result.player_et <= 0.0 || result.opponent_et <= 0.0)
        return fail("race elapsed times were not generated");
    if (result.wear <= 0) return fail("race did not generate vehicle wear");
    if (game.active_car() == nullptr || game.active_car()->condition >= 100)
        return fail("race wear did not reduce vehicle condition");
    if (game.wins() + game.losses() != 1) return fail("race record was not updated");

    const int damaged_value = game.active_car()->resale_value();
    const int repair_quote = game.active_car()->repair_cost();
    if (repair_quote <= 0) return fail("damaged car has no repair quote");

    int paid = 0;
    if (!game.repair_active_car(&paid, &error)) return fail("could not repair damaged car: " + error);
    if (paid != repair_quote) return fail("repair charged a different amount than quoted");
    if (game.active_car() == nullptr || game.active_car()->condition != 100)
        return fail("repair did not restore full condition");
    if (game.active_car()->resale_value() <= damaged_value)
        return fail("repair did not restore resale value");

    const int cash_before_sale = game.cash();
    const int expected_sale = game.active_car()->resale_value();
    int sale_price = 0;
    if (!game.sell_active_car(&sale_price, &error)) return fail("could not sell active car: " + error);
    if (sale_price != expected_sale) return fail("car sale price did not match displayed resale value");
    if (game.cash() != cash_before_sale + sale_price) return fail("car sale did not credit cash correctly");
    if (!game.garage().empty()) return fail("sold car remained in garage");

    std::cout << "gameplay smoke test passed\n";
    return 0;
}
