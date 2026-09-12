// SPDX-License-Identifier: GPL-3.0-or-later
// Presentation-art adapter.
//
// The drag strip keeps the long top-down Racing Pack sprites because they read
// correctly in horizontal lanes. Presentation screens use isometric civilian
// cars so the garage, classifieds, diner and menu no longer show race-view art.
// The classifieds deliberately rotate through multiple real Kenney body styles
// and the upstream pack's civilian colour families so it reads as a used-car
// market rather than a row of cloned placeholders.
//
// Screen is declared inside main.cpp, so the selector is templated and compares
// its stable enum ordinal: Menu=0, Classifieds=1, Garage=2, Parts=3, Diner=4,
// Race=5. No gameplay or timing behaviour is changed by this translation unit.

#include <iterator>

#include "generated_assets.h"
#include "generated_isometric_assets.h"
#include "generated_vehicle_palette_variants.h"
#include "generated_vehicle_body_variants.h"

namespace backyard_racer::assets {

inline constexpr const EmbeddedSprite& race_red_car = kenney_red_car;
inline constexpr const EmbeddedSprite& race_blue_car = kenney_blue_car;

inline const EmbeddedSprite& classifieds_car(int cx, int base_y) {
    const unsigned slot = static_cast<unsigned>((cx / 90 + base_y / 70) % 8);
    switch (slot) {
        case 0: return kenney_iso_red_sedan;
        case 1: return kenney_iso_blue_sedan;
        case 2: return kenney_iso_red_sedan2;
        case 3: return kenney_iso_silver_sedan;
        case 4: return kenney_iso_red_pickup;
        case 5: return kenney_iso_green_sedan;
        case 6: return kenney_iso_black_sedan;
        default: return kenney_iso_red_sedan2;
    }
}

template <typename ScreenType>
inline const EmbeddedSprite& presentation_red_car(ScreenType screen, int cx, int base_y) {
    const int value = static_cast<int>(screen);
    if (value == 5) return race_red_car;
    if (value == 2) return kenney_iso_red_sedan_garage;
    if (value == 1) return classifieds_car(cx, base_y);
    return kenney_iso_red_sedan;
}

template <typename ScreenType>
inline const EmbeddedSprite& presentation_blue_car(ScreenType screen, int cx, int base_y) {
    const int value = static_cast<int>(screen);
    if (value == 5) return race_blue_car;
    if (value == 2) return kenney_iso_blue_sedan_garage;
    if (value == 1) return classifieds_car(cx, base_y);
    if (value == 4) return kenney_iso_silver_sedan;
    return kenney_iso_blue_sedan;
}

} // namespace backyard_racer::assets

#define kenney_red_car presentation_red_car(screen_, cx, base_y)
#define kenney_blue_car presentation_blue_car(screen_, cx, base_y)
#include "main.cpp"
