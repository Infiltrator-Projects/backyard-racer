// SPDX-License-Identifier: GPL-3.0-or-later
#include "gameplay.h"
#include "vehicle_data.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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
    if (game.current_week() != 1) return fail("new game did not start in week one");
    if (game.vehicle_catalog().size() < 50) return fail("vehicle variant catalog is too small");
    if (game.engine_catalog().size() < 30) return fail("engine catalog is too small");
    if (game.classifieds().size() != 8) return fail("weekly classifieds did not generate eight ads");

    int starter_count = 0;
    std::vector<std::string> week_one;
    for (const auto& car : game.classifieds()) {
        if (car.starter_ok) ++starter_count;
        week_one.push_back(car.variant_id + ":" + std::to_string(car.price));
    }
    if (starter_count < 2) return fail("weekly market did not protect starter choices");

    game.advance_week();
    if (game.current_week() != 2) return fail("week did not advance");
    if (game.classifieds().size() != 8) return fail("week two market did not regenerate eight ads");
    std::vector<std::string> week_two;
    for (const auto& car : game.classifieds())
        week_two.push_back(car.variant_id + ":" + std::to_string(car.price));
    if (week_one == week_two) return fail("weekly market did not change");

    std::size_t buy_index = game.classifieds().size();
    int cheapest = 1'000'000;
    for (std::size_t i = 0; i < game.classifieds().size(); ++i) {
        if (game.classifieds()[i].price <= game.cash() && game.classifieds()[i].price < cheapest) {
            cheapest = game.classifieds()[i].price;
            buy_index = i;
        }
    }
    if (buy_index == game.classifieds().size()) return fail("weekly market has no affordable car");
    const CarSpec bought_spec = game.classifieds()[buy_index];
    const int cash_before_car = game.cash();

    std::string error;
    if (!game.buy_car(buy_index, &error)) return fail("could not buy first car: " + error);
    if (game.active_car() == nullptr) return fail("bought car was not added to garage");
    if (game.cash() != cash_before_car - bought_spec.price)
        return fail("car purchase did not debit cash correctly");
    if (game.classifieds().size() != 7) return fail("sold classified remained in the paper");
    if (game.active_car()->condition != 100) return fail("newly bought car did not start at full condition");
    if (game.active_car()->base.variant_id.empty() || game.active_car()->base.engine_id.empty())
        return fail("bought car lost variant or engine identity");

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

    const std::string chassis = game.active_car()->base.chassis_family;
    const EngineSpec* incompatible = nullptr;
    for (const auto& engine : game.engine_catalog()) {
        if (!engine_family_fits_chassis(chassis, engine.family)) {
            incompatible = &engine;
            break;
        }
    }
    if (!incompatible) return fail("could not find an incompatible engine for fitment test");
    const int cash_before_reject = game.cash();
    if (game.swap_engine(incompatible->id, nullptr, &error))
        return fail("cross-family engine swap was incorrectly accepted");
    if (game.cash() != cash_before_reject) return fail("rejected engine swap charged cash");

    const auto compatible = game.compatible_engines();
    const EngineSpec* replacement = nullptr;
    for (const auto& engine : compatible) {
        if (engine.price <= game.cash()) {
            replacement = &engine;
            break;
        }
    }
    if (!replacement) return fail("no affordable compatible engine was offered");
    const std::string replacement_id = replacement->id;
    const int replacement_hp = replacement->horsepower;
    const int cash_before_engine = game.cash();
    int engine_price = 0;
    if (!game.swap_engine(replacement_id, &engine_price, &error))
        return fail("compatible engine swap failed: " + error);
    if (!game.active_car() || game.active_car()->base.engine_id != replacement_id)
        return fail("engine identity did not change after swap");
    if (game.active_car()->base.horsepower != replacement_hp)
        return fail("engine swap did not set the engine horsepower");
    if (game.cash() != cash_before_engine - engine_price)
        return fail("engine swap did not charge the engine price");
    if (game.spare_parts().size() < 2)
        return fail("removed engine was not returned to the parts bin");

    const auto save_path = std::filesystem::temp_directory_path() / "backyard-racer-gameplay-test.sav";
    std::error_code remove_error;
    std::filesystem::remove(save_path, remove_error);
    if (!game.save_to(save_path.string(), &error)) return fail("could not save game: " + error);

    GameState restored;
    if (!restored.load_from(save_path.string(), &error)) return fail("could not reload game: " + error);
    std::filesystem::remove(save_path, remove_error);
    if (!restored.started()) return fail("reloaded game was not marked started");
    if (restored.cash() != game.cash()) return fail("cash did not survive save/load");
    if (restored.current_week() != game.current_week()) return fail("week did not survive save/load");
    if (restored.classifieds().size() != game.classifieds().size())
        return fail("weekly classifieds did not survive save/load");
    if (restored.garage().size() != game.garage().size()) return fail("garage did not survive save/load");
    if (restored.spare_parts().size() != game.spare_parts().size()) return fail("parts bin did not survive save/load");
    if (!restored.active_car() || restored.active_car()->base.variant_id != game.active_car()->base.variant_id)
        return fail("active car variant did not survive save/load");
    if (restored.active_car()->base.engine_id != game.active_car()->base.engine_id)
        return fail("engine swap did not survive save/load");
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
