// SPDX-License-Identifier: GPL-3.0-or-later
// Presentation-art adapter.
//
// The drag strip keeps the long top-down Racing Pack sprites because they read
// correctly in horizontal lanes. Presentation screens use isometric civilian
// cars so the garage, classifieds, diner and menu no longer show race-view art.
// The garage uses a padded variant to keep the diagonal sprite inside the bay.
//
// Screen is declared inside main.cpp, so the selector is templated and compares
// its stable enum ordinal: Menu=0, Classifieds=1, Garage=2, Parts=3, Diner=4,
// Race=5. No gameplay or timing behaviour is changed by this translation unit.

#include <iterator>

#include "generated_assets.h"
#include "generated_isometric_assets.h"

namespace backyard_racer::assets {

inline constexpr const EmbeddedSprite& race_red_car = kenney_red_car;
inline constexpr const EmbeddedSprite& race_blue_car = kenney_blue_car;

template <typename ScreenType>
inline const EmbeddedSprite& presentation_red_car(ScreenType screen) {
    const int value = static_cast<int>(screen);
    if (value == 5) return race_red_car;
    if (value == 2) return kenney_iso_red_sedan_garage;
    return kenney_iso_red_sedan;
}

template <typename ScreenType>
inline const EmbeddedSprite& presentation_blue_car(ScreenType screen) {
    const int value = static_cast<int>(screen);
    if (value == 5) return race_blue_car;
    if (value == 2) return kenney_iso_blue_sedan_garage;
    return kenney_iso_blue_sedan;
}

} // namespace backyard_racer::assets

#define kenney_red_car presentation_red_car(screen_)
#define kenney_blue_car presentation_blue_car(screen_)
#include "main.cpp"
