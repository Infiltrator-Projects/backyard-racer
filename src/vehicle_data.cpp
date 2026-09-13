// SPDX-License-Identifier: GPL-3.0-or-later
#include "vehicle_data.h"

#include <algorithm>
#include <initializer_list>
#include <string>

namespace backyard_racer {
namespace {

EngineSpec engine(const char* id, const char* maker, const char* family,
                  const char* name, const char* layout,
                  int displacement_ci, int horsepower, int weight_lb, int price) {
    EngineSpec e;
    e.id = id;
    e.manufacturer = maker;
    e.family = family;
    e.name = name;
    e.layout = layout;
    e.displacement_ci = displacement_ci;
    e.horsepower = horsepower;
    e.weight_lb = weight_lb;
    e.price = price;
    return e;
}

CarSpec car(const char* body_id, const char* variant_id,
            int year, const char* make, const char* model, const char* variant,
            int price, int horsepower, int weight_lb, double traction, int gears,
            const char* chassis_family, const char* engine_id,
            int rarity_weight, bool starter_ok) {
    CarSpec c;
    c.id = body_id; // body/art identity deliberately shared by engine variants
    c.year = year;
    c.make = make;
    c.model = model;
    c.price = price;
    c.horsepower = horsepower;
    c.weight_lb = weight_lb;
    c.traction = traction;
    c.gears = gears;
    c.variant_id = variant_id;
    c.variant = variant;
    c.chassis_family = chassis_family;
    c.engine_id = engine_id;
    c.rarity_weight = rarity_weight;
    c.starter_ok = starter_ok;
    return c;
}

bool is_one_of(std::string_view value, std::initializer_list<std::string_view> choices) {
    return std::find(choices.begin(), choices.end(), value) != choices.end();
}

} // namespace

const std::vector<EngineSpec>& historical_engine_catalog() {
    // Horsepower/displacement are period SAE-gross catalogue values. Engine
    // weights and prices are simulation values used for swap weight/cost.
    static const std::vector<EngineSpec> engines = {
        // Ford small six / Windsor / FE families.
        engine("ford_170_i6_101", "FORD", "ford_small_six", "170 I6", "I6", 170, 101, 385, 350),
        engine("ford_200_i6_116", "FORD", "ford_small_six", "200 I6", "I6", 200, 116, 410, 425),
        engine("ford_200_i6_120", "FORD", "ford_small_six", "200 I6", "I6", 200, 120, 410, 450),
        engine("ford_250_i6_155", "FORD", "ford_small_six", "250 I6", "I6", 250, 155, 430, 575),
        engine("ford_260_v8_164", "FORD", "ford_windsor", "260 2V V8", "V8", 260, 164, 480, 725),
        engine("ford_289_2v_200", "FORD", "ford_windsor", "289 2V V8", "V8", 289, 200, 490, 850),
        engine("ford_289_4v_225", "FORD", "ford_windsor", "289 4V V8", "V8", 289, 225, 490, 1000),
        engine("ford_289_hipo_271", "FORD", "ford_windsor", "289 HiPo V8", "V8", 289, 271, 495, 1350),
        engine("ford_302_2v_210", "FORD", "ford_windsor", "302 2V V8", "V8", 302, 210, 500, 950),
        engine("ford_302_4v_230", "FORD", "ford_windsor", "302 4V V8", "V8", 302, 230, 500, 1100),
        engine("ford_302_2v_220", "FORD", "ford_windsor", "302 2V V8", "V8", 302, 220, 500, 1000),
        engine("ford_351w_2v_250", "FORD", "ford_windsor", "351W 2V V8", "V8", 351, 250, 525, 1250),
        engine("ford_351w_4v_290", "FORD", "ford_windsor", "351W 4V V8", "V8", 351, 290, 525, 1550),
        engine("ford_390_fe_280", "FORD", "ford_fe", "390 2V FE V8", "V8", 390, 280, 625, 1650),
        engine("ford_390_fe_320", "FORD", "ford_fe", "390 4V FE V8", "V8", 390, 320, 625, 1900),
        engine("ford_390_fe_325", "FORD", "ford_fe", "390 GT FE V8", "V8", 390, 325, 625, 2050),
        engine("ford_427_fe_390", "FORD", "ford_fe", "427 4V FE V8", "V8", 427, 390, 650, 3300),
        engine("ford_428_cj_335", "FORD", "ford_fe", "428 Cobra Jet V8", "V8", 428, 335, 650, 2700),

        // Chevrolet inline-six, small-block and big-block families.
        engine("chevy_194_i6_120", "CHEVROLET", "chevy_inline6", "194 I6", "I6", 194, 120, 430, 375),
        engine("chevy_230_i6_140", "CHEVROLET", "chevy_inline6", "230 I6", "I6", 230, 140, 440, 475),
        engine("chevy_250_i6_155", "CHEVROLET", "chevy_inline6", "250 I6", "I6", 250, 155, 450, 550),
        engine("chevy_283_2v_195", "CHEVROLET", "chevy_small_block", "283 2V V8", "V8", 283, 195, 575, 800),
        engine("chevy_283_4v_220", "CHEVROLET", "chevy_small_block", "283 4V V8", "V8", 283, 220, 575, 950),
        engine("chevy_302_z28_290", "CHEVROLET", "chevy_small_block", "302 Z/28 V8", "V8", 302, 290, 575, 1800),
        engine("chevy_307_2v_200", "CHEVROLET", "chevy_small_block", "307 2V V8", "V8", 307, 200, 575, 825),
        engine("chevy_327_2v_210", "CHEVROLET", "chevy_small_block", "327 2V V8", "V8", 327, 210, 575, 900),
        engine("chevy_327_4v_275", "CHEVROLET", "chevy_small_block", "327 4V V8", "V8", 327, 275, 575, 1250),
        engine("chevy_327_4v_350", "CHEVROLET", "chevy_small_block", "327 HiPo V8", "V8", 327, 350, 575, 1900),
        engine("chevy_350_2v_250", "CHEVROLET", "chevy_small_block", "350 2V V8", "V8", 350, 250, 585, 1150),
        engine("chevy_350_4v_295", "CHEVROLET", "chevy_small_block", "350 4V V8", "V8", 350, 295, 585, 1450),
        engine("chevy_350_4v_300", "CHEVROLET", "chevy_small_block", "350 4V V8", "V8", 350, 300, 585, 1500),
        engine("chevy_396_4v_325", "CHEVROLET", "chevy_big_block", "396 4V V8", "V8", 396, 325, 685, 1900),
        engine("chevy_396_4v_350", "CHEVROLET", "chevy_big_block", "396 4V V8", "V8", 396, 350, 685, 2150),
        engine("chevy_396_4v_375", "CHEVROLET", "chevy_big_block", "396 L78 V8", "V8", 396, 375, 690, 2500),
        engine("chevy_402_4v_350", "CHEVROLET", "chevy_big_block", "402 V8", "V8", 402, 350, 690, 2200),
        engine("chevy_454_4v_360", "CHEVROLET", "chevy_big_block", "454 LS5 V8", "V8", 454, 360, 700, 2700),
        engine("chevy_454_4v_450", "CHEVROLET", "chevy_big_block", "454 LS6 V8", "V8", 454, 450, 705, 3900),

        // Chrysler Slant-Six, LA, B/RB and Hemi families.
        engine("mopar_225_i6_145", "CHRYSLER", "mopar_slant6", "225 Slant-Six", "I6", 225, 145, 475, 500),
        engine("mopar_318_v8_230", "CHRYSLER", "mopar_la", "318 2V V8", "V8", 318, 230, 525, 950),
        engine("mopar_340_v8_275", "CHRYSLER", "mopar_la", "340 4V V8", "V8", 340, 275, 540, 1450),
        engine("mopar_383_2v_290", "CHRYSLER", "mopar_brb", "383 2V V8", "V8", 383, 290, 650, 1600),
        engine("mopar_383_4v_300", "CHRYSLER", "mopar_brb", "383 4V V8", "V8", 383, 300, 650, 1750),
        engine("mopar_383_hp_335", "CHRYSLER", "mopar_brb", "383 Road Runner V8", "V8", 383, 335, 650, 2050),
        engine("mopar_440_4v_375", "CHRYSLER", "mopar_brb", "440 Magnum V8", "V8", 440, 375, 670, 2600),
        engine("mopar_440_6_390", "CHRYSLER", "mopar_brb", "440 Six Pack V8", "V8", 440, 390, 675, 3150),
        engine("mopar_426_hemi_425", "CHRYSLER", "mopar_hemi", "426 Street Hemi V8", "V8", 426, 425, 765, 4300),

        // Pontiac 400 family (Pontiac did not use Chevy small/big-block families).
        engine("pontiac_400_2v_265", "PONTIAC", "pontiac_v8", "400 2V V8", "V8", 400, 265, 650, 1500),
        engine("pontiac_400_4v_350", "PONTIAC", "pontiac_v8", "400 4V V8", "V8", 400, 350, 650, 2100),
        engine("pontiac_400_ra3_366", "PONTIAC", "pontiac_v8", "400 Ram Air III V8", "V8", 400, 366, 650, 2550),
        engine("pontiac_400_ra4_370", "PONTIAC", "pontiac_v8", "400 Ram Air IV V8", "V8", 400, 370, 655, 3000),
    };
    return engines;
}

const std::vector<CarSpec>& historical_vehicle_variants() {
    // Purchase prices are Backyard Racer economy values, not historical MSRP.
    // Powertrain combinations are based on period model-year offerings.
    static const std::vector<CarSpec> cars = {
        // 1964 Ford Falcon: same body art, materially different drivetrains.
        car("falcon64", "falcon64_170", 1964, "FORD", "FALCON", "170 I6", 900, 101, 2640, 0.86, 3, "ford_falcon64", "ford_170_i6_101", 130, true),
        car("falcon64", "falcon64_200", 1964, "FORD", "FALCON", "200 I6", 1050, 116, 2670, 0.87, 3, "ford_falcon64", "ford_200_i6_116", 95, true),
        car("falcon64", "falcon64_260", 1964, "FORD", "FALCON SPRINT", "260 2V V8", 1450, 164, 2760, 0.89, 4, "ford_falcon64", "ford_260_v8_164", 70, true),

        // 1965 Ford Mustang.
        car("mustang65", "mustang65_200", 1965, "FORD", "MUSTANG", "200 I6", 1400, 120, 2700, 0.88, 3, "ford_mustang65", "ford_200_i6_120", 120, true),
        car("mustang65", "mustang65_289c", 1965, "FORD", "MUSTANG", "289 2V V8", 1800, 200, 2860, 0.90, 3, "ford_mustang65", "ford_289_2v_200", 100, true),
        car("mustang65", "mustang65_289a", 1965, "FORD", "MUSTANG", "289 4V V8", 2200, 225, 2890, 0.91, 4, "ford_mustang65", "ford_289_4v_225", 70, true),
        car("mustang65", "mustang65_289k", 1965, "FORD", "MUSTANG K-CODE", "289 HiPo V8", 3200, 271, 2920, 0.93, 4, "ford_mustang65", "ford_289_hipo_271", 22, false),

        // 1966 Chevrolet Chevy II / Nova.
        car("nova66", "nova66_194", 1966, "CHEVROLET", "NOVA", "194 I6", 1200, 120, 2640, 0.87, 3, "chevy_nova66", "chevy_194_i6_120", 120, true),
        car("nova66", "nova66_230", 1966, "CHEVROLET", "NOVA", "230 I6", 1350, 140, 2670, 0.88, 3, "chevy_nova66", "chevy_230_i6_140", 90, true),
        car("nova66", "nova66_283_2v", 1966, "CHEVROLET", "NOVA", "283 2V V8", 1800, 195, 2740, 0.90, 3, "chevy_nova66", "chevy_283_2v_195", 90, true),
        car("nova66", "nova66_283_4v", 1966, "CHEVROLET", "NOVA", "283 4V V8", 2150, 220, 2760, 0.91, 4, "chevy_nova66", "chevy_283_4v_220", 65, true),
        car("nova66", "nova66_327_275", 1966, "CHEVROLET", "NOVA", "327 4V V8", 2700, 275, 2800, 0.93, 4, "chevy_nova66", "chevy_327_4v_275", 35, false),
        car("nova66", "nova66_327_350", 1966, "CHEVROLET", "NOVA SS", "327 HiPo V8", 3600, 350, 2820, 0.95, 4, "chevy_nova66", "chevy_327_4v_350", 10, false),

        // 1967 Chevrolet Camaro.
        car("camaro67", "camaro67_230", 1967, "CHEVROLET", "CAMARO", "230 I6", 1800, 140, 2940, 0.89, 3, "chevy_camaro67", "chevy_230_i6_140", 110, true),
        car("camaro67", "camaro67_250", 1967, "CHEVROLET", "CAMARO", "250 I6", 1950, 155, 2970, 0.89, 3, "chevy_camaro67", "chevy_250_i6_155", 95, true),
        car("camaro67", "camaro67_327_210", 1967, "CHEVROLET", "CAMARO", "327 2V V8", 2400, 210, 3070, 0.91, 3, "chevy_camaro67", "chevy_327_2v_210", 90, true),
        car("camaro67", "camaro67_327_275", 1967, "CHEVROLET", "CAMARO", "327 4V V8", 2900, 275, 3100, 0.93, 4, "chevy_camaro67", "chevy_327_4v_275", 55, false),
        car("camaro67", "camaro67_350_295", 1967, "CHEVROLET", "CAMARO SS", "350 4V V8", 3400, 295, 3140, 0.94, 4, "chevy_camaro67", "chevy_350_4v_295", 40, false),
        car("camaro67", "camaro67_396_325", 1967, "CHEVROLET", "CAMARO SS", "396 4V V8", 4400, 325, 3270, 0.95, 4, "chevy_camaro67", "chevy_396_4v_325", 22, false),
        car("camaro67", "camaro67_396_375", 1967, "CHEVROLET", "CAMARO SS", "396 L78 V8", 5900, 375, 3290, 0.96, 4, "chevy_camaro67", "chevy_396_4v_375", 7, false),
        car("camaro67", "camaro67_z28", 1967, "CHEVROLET", "CAMARO Z/28", "302 V8", 6500, 290, 3040, 0.97, 4, "chevy_camaro67", "chevy_302_z28_290", 4, false),

        // 1968 Dodge Charger.
        car("charger68", "charger68_318", 1968, "DODGE", "CHARGER", "318 2V V8", 2800, 230, 3550, 0.92, 3, "mopar_charger68", "mopar_318_v8_230", 95, false),
        car("charger68", "charger68_383_2v", 1968, "DODGE", "CHARGER", "383 2V V8", 3300, 290, 3650, 0.93, 3, "mopar_charger68", "mopar_383_2v_290", 65, false),
        car("charger68", "charger68_383_4v", 1968, "DODGE", "CHARGER", "383 4V V8", 3700, 300, 3670, 0.94, 4, "mopar_charger68", "mopar_383_4v_300", 45, false),
        car("charger68", "charger68_440", 1968, "DODGE", "CHARGER R/T", "440 Magnum V8", 4700, 375, 3720, 0.96, 4, "mopar_charger68", "mopar_440_4v_375", 20, false),
        car("charger68", "charger68_hemi", 1968, "DODGE", "CHARGER R/T", "426 Hemi V8", 7600, 425, 3820, 0.98, 4, "mopar_charger68", "mopar_426_hemi_425", 2, false),

        // 1969 Pontiac GTO.
        car("gto69", "gto69_400_265", 1969, "PONTIAC", "GTO", "400 2V V8", 3000, 265, 3500, 0.93, 3, "pontiac_gto69", "pontiac_400_2v_265", 65, false),
        car("gto69", "gto69_400_350", 1969, "PONTIAC", "GTO", "400 4V V8", 3800, 350, 3580, 0.95, 4, "pontiac_gto69", "pontiac_400_4v_350", 70, false),
        car("gto69", "gto69_ra3", 1969, "PONTIAC", "GTO RAM AIR III", "400 V8", 4700, 366, 3600, 0.96, 4, "pontiac_gto69", "pontiac_400_ra3_366", 22, false),
        car("gto69", "gto69_ra4", 1969, "PONTIAC", "GTO RAM AIR IV", "400 V8", 5800, 370, 3610, 0.97, 4, "pontiac_gto69", "pontiac_400_ra4_370", 7, false),

        // 1969 Plymouth Road Runner.
        car("roadrunner69", "roadrunner69_383", 1969, "PLYMOUTH", "ROAD RUNNER", "383 HP V8", 3400, 335, 3450, 0.95, 4, "mopar_roadrunner69", "mopar_383_hp_335", 90, false),
        car("roadrunner69", "roadrunner69_a12", 1969, "PLYMOUTH", "ROAD RUNNER A12", "440 Six Barrel V8", 5200, 390, 3550, 0.97, 4, "mopar_roadrunner69", "mopar_440_6_390", 14, false),
        car("roadrunner69", "roadrunner69_hemi", 1969, "PLYMOUTH", "ROAD RUNNER", "426 Hemi V8", 7500, 425, 3650, 0.98, 4, "mopar_roadrunner69", "mopar_426_hemi_425", 3, false),

        // 1970 Dodge Challenger.
        car("challenger70", "challenger70_225", 1970, "DODGE", "CHALLENGER", "225 Slant-Six", 2600, 145, 3220, 0.90, 3, "mopar_challenger70", "mopar_225_i6_145", 80, false),
        car("challenger70", "challenger70_318", 1970, "DODGE", "CHALLENGER", "318 2V V8", 3000, 230, 3340, 0.92, 3, "mopar_challenger70", "mopar_318_v8_230", 90, false),
        car("challenger70", "challenger70_340", 1970, "DODGE", "CHALLENGER", "340 4V V8", 3800, 275, 3380, 0.94, 4, "mopar_challenger70", "mopar_340_v8_275", 55, false),
        car("challenger70", "challenger70_383", 1970, "DODGE", "CHALLENGER R/T", "383 Magnum V8", 4200, 335, 3500, 0.95, 4, "mopar_challenger70", "mopar_383_hp_335", 42, false),
        car("challenger70", "challenger70_440", 1970, "DODGE", "CHALLENGER R/T", "440 Magnum V8", 5300, 375, 3550, 0.96, 4, "mopar_challenger70", "mopar_440_4v_375", 18, false),
        car("challenger70", "challenger70_440_6", 1970, "DODGE", "CHALLENGER R/T", "440 Six Pack V8", 6200, 390, 3570, 0.97, 4, "mopar_challenger70", "mopar_440_6_390", 9, false),
        car("challenger70", "challenger70_hemi", 1970, "DODGE", "CHALLENGER R/T", "426 Hemi V8", 8000, 425, 3660, 0.98, 4, "mopar_challenger70", "mopar_426_hemi_425", 2, false),

        // 1968 Mercury Cougar: same Ford engine families, different body.
        car("cougar68", "cougar68_302_2v", 1968, "MERCURY", "COUGAR", "302 2V V8", 2400, 210, 3270, 0.92, 3, "ford_cougar68", "ford_302_2v_210", 95, true),
        car("cougar68", "cougar68_302_4v", 1968, "MERCURY", "COUGAR", "302 4V V8", 2800, 230, 3290, 0.93, 4, "ford_cougar68", "ford_302_4v_230", 65, false),
        car("cougar68", "cougar68_390_2v", 1968, "MERCURY", "COUGAR", "390 2V FE V8", 3300, 280, 3400, 0.94, 3, "ford_cougar68", "ford_390_fe_280", 35, false),
        car("cougar68", "cougar68_390_4v", 1968, "MERCURY", "COUGAR GT", "390 GT FE V8", 3900, 325, 3440, 0.95, 4, "ford_cougar68", "ford_390_fe_325", 25, false),
        car("cougar68", "cougar68_427", 1968, "MERCURY", "COUGAR GT-E", "427 FE V8", 7200, 390, 3500, 0.98, 3, "ford_cougar68", "ford_427_fe_390", 2, false),
        car("cougar68", "cougar68_428", 1968, "MERCURY", "COUGAR GT-E", "428 Cobra Jet V8", 5600, 335, 3500, 0.97, 4, "ford_cougar68", "ford_428_cj_335", 8, false),

        // 1969 Ford Torino / Fairlane intermediate family.
        car("torino69", "torino69_250", 1969, "FORD", "TORINO", "250 I6", 1700, 155, 3200, 0.89, 3, "ford_torino69", "ford_250_i6_155", 95, true),
        car("torino69", "torino69_302", 1969, "FORD", "TORINO GT", "302 2V V8", 2200, 220, 3310, 0.91, 3, "ford_torino69", "ford_302_2v_220", 90, true),
        car("torino69", "torino69_351_2v", 1969, "FORD", "TORINO", "351W 2V V8", 2800, 250, 3360, 0.92, 3, "ford_torino69", "ford_351w_2v_250", 65, false),
        car("torino69", "torino69_351_4v", 1969, "FORD", "TORINO GT", "351W 4V V8", 3400, 290, 3380, 0.94, 4, "ford_torino69", "ford_351w_4v_290", 38, false),
        car("torino69", "torino69_390", 1969, "FORD", "TORINO GT", "390 4V FE V8", 4000, 320, 3490, 0.95, 4, "ford_torino69", "ford_390_fe_320", 22, false),
        car("torino69", "torino69_428", 1969, "FORD", "COBRA", "428 Cobra Jet V8", 5400, 335, 3520, 0.97, 4, "ford_torino69", "ford_428_cj_335", 9, false),

        // 1970 Chevrolet Chevelle.
        car("chevelle70", "chevelle70_250", 1970, "CHEVROLET", "CHEVELLE", "250 I6", 2200, 155, 3300, 0.89, 3, "chevy_chevelle70", "chevy_250_i6_155", 85, true),
        car("chevelle70", "chevelle70_307", 1970, "CHEVROLET", "CHEVELLE", "307 2V V8", 2500, 200, 3380, 0.91, 3, "chevy_chevelle70", "chevy_307_2v_200", 95, false),
        car("chevelle70", "chevelle70_350_250", 1970, "CHEVROLET", "CHEVELLE", "350 V8", 2900, 250, 3420, 0.92, 3, "chevy_chevelle70", "chevy_350_2v_250", 70, false),
        car("chevelle70", "chevelle70_350_300", 1970, "CHEVROLET", "CHEVELLE", "350 4V V8", 3400, 300, 3450, 0.94, 4, "chevy_chevelle70", "chevy_350_4v_300", 45, false),
        car("chevelle70", "chevelle70_402", 1970, "CHEVROLET", "CHEVELLE SS", "402 V8", 4300, 350, 3570, 0.95, 4, "chevy_chevelle70", "chevy_402_4v_350", 24, false),
        car("chevelle70", "chevelle70_454_ls5", 1970, "CHEVROLET", "CHEVELLE SS", "454 LS5 V8", 5200, 360, 3650, 0.96, 4, "chevy_chevelle70", "chevy_454_4v_360", 14, false),
        car("chevelle70", "chevelle70_454_ls6", 1970, "CHEVROLET", "CHEVELLE SS", "454 LS6 V8", 7200, 450, 3670, 0.98, 4, "chevy_chevelle70", "chevy_454_4v_450", 3, false),

        // 1970 Plymouth Barracuda / 'Cuda.
        car("barracuda70", "barracuda70_225", 1970, "PLYMOUTH", "BARRACUDA", "225 Slant-Six", 2500, 145, 3210, 0.90, 3, "mopar_barracuda70", "mopar_225_i6_145", 85, false),
        car("barracuda70", "barracuda70_318", 1970, "PLYMOUTH", "BARRACUDA", "318 2V V8", 2900, 230, 3300, 0.92, 3, "mopar_barracuda70", "mopar_318_v8_230", 90, false),
        car("barracuda70", "barracuda70_340", 1970, "PLYMOUTH", "'CUDA", "340 4V V8", 3700, 275, 3340, 0.94, 4, "mopar_barracuda70", "mopar_340_v8_275", 50, false),
        car("barracuda70", "barracuda70_383", 1970, "PLYMOUTH", "'CUDA", "383 V8", 4200, 335, 3460, 0.95, 4, "mopar_barracuda70", "mopar_383_hp_335", 34, false),
        car("barracuda70", "barracuda70_440", 1970, "PLYMOUTH", "'CUDA", "440 V8", 5200, 375, 3520, 0.96, 4, "mopar_barracuda70", "mopar_440_4v_375", 16, false),
        car("barracuda70", "barracuda70_440_6", 1970, "PLYMOUTH", "'CUDA", "440 Six Barrel V8", 6100, 390, 3540, 0.97, 4, "mopar_barracuda70", "mopar_440_6_390", 8, false),
        car("barracuda70", "barracuda70_hemi", 1970, "PLYMOUTH", "HEMI 'CUDA", "426 Hemi V8", 7900, 425, 3640, 0.98, 4, "mopar_barracuda70", "mopar_426_hemi_425", 2, false),
    };
    return cars;
}

const EngineSpec* find_engine_spec(std::string_view id) {
    const auto& engines = historical_engine_catalog();
    const auto it = std::find_if(engines.begin(), engines.end(),
        [&](const EngineSpec& e) { return e.id == id; });
    return it == engines.end() ? nullptr : &*it;
}

bool engine_family_fits_chassis(std::string_view chassis, std::string_view family) {
    // Conservative fitment: families that were factory-installed in that body
    // architecture (or the directly shared Ford family), not arbitrary custom
    // fabrication. Cross-manufacturer swaps are deliberately rejected.
    if (chassis == "ford_falcon64" || chassis == "ford_mustang65")
        return is_one_of(family, {"ford_small_six", "ford_windsor"});
    if (chassis == "ford_cougar68")
        return is_one_of(family, {"ford_windsor", "ford_fe"});
    if (chassis == "ford_torino69")
        return is_one_of(family, {"ford_small_six", "ford_windsor", "ford_fe"});

    if (chassis == "chevy_nova66")
        return is_one_of(family, {"chevy_inline6", "chevy_small_block"});
    if (chassis == "chevy_camaro67" || chassis == "chevy_chevelle70")
        return is_one_of(family, {"chevy_inline6", "chevy_small_block", "chevy_big_block"});
    if (chassis == "pontiac_gto69")
        return family == "pontiac_v8";

    if (chassis == "mopar_charger68")
        return is_one_of(family, {"mopar_la", "mopar_brb", "mopar_hemi"});
    if (chassis == "mopar_roadrunner69")
        return is_one_of(family, {"mopar_brb", "mopar_hemi"});
    if (chassis == "mopar_challenger70" || chassis == "mopar_barracuda70")
        return is_one_of(family, {"mopar_slant6", "mopar_la", "mopar_brb", "mopar_hemi"});

    return false;
}

} // namespace backyard_racer
