// SPDX-License-Identifier: GPL-3.0-or-later
#include "gameplay.h"

#include <filesystem>
#include <string>

int main() {
    using namespace backyard_racer;
    GameState game;
    game.new_game();
    std::string error;
    if (!game.buy_car(0, &error) || !game.active_car()) return 1;
    if (game.active_car()->paint.name.empty()) return 2;

    if (!game.repaint_active_car(12, 34, 56, "TEST CUSTOM", true, &error)) return 3;
    const auto path = std::filesystem::temp_directory_path() / "backyard-racer-paint-test.sav";
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (!game.save_to(path.string(), &error)) return 4;

    GameState restored;
    if (!restored.load_from(path.string(), &error)) return 5;
    std::filesystem::remove(path, ec);
    if (!restored.active_car()) return 6;
    const PaintState& paint = restored.active_car()->paint;
    if (paint.r != 12 || paint.g != 34 || paint.b != 56) return 7;
    if (paint.name != "TEST CUSTOM" || !paint.custom) return 8;
    return 0;
}
