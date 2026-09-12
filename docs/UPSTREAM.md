# Upstream code and research used to accelerate Backyard Racer

Backyard Racer is a modern native implementation. The following public projects are used as upstream engineering references so proven Street Rod-style systems are not needlessly redesigned.

## StreetRod3/StreetRod3Classic

Repository: `StreetRod3/StreetRod3Classic`

Relevant source files explicitly carry GNU GPL version 2 or, at the user's option, any later version notices. Backyard Racer is GPL-3.0-or-later, so compatible ideas and ported implementation work may be incorporated with attribution.

Useful systems reviewed:

- `Garage.cpp` and `Garage.h` — garage state and interaction architecture.
- `Garage_Newspaper.cpp` — newspaper front page, used-car listings, parts listings and purchase flow.
- `Gar_Parts.cpp` — fitted-part removal/addition flow and repair state.
- `CCar.cpp`, `CCar_Driving.cpp`, `CCarSim.cpp`, `CarSimulation.cpp` — car state and driving simulation.
- `Diner.cpp`, `COpponent.cpp`, `CPlayer.cpp` — player/opponent/race challenge structure.
- `Race_main.cpp` and `racing.h` — race state and execution.

The current Backyard Racer gameplay core is a fresh C++20 implementation informed by that separation of concerns rather than a wholesale import of the old SDL/OpenGL engine.

## herbert3000/StreetRodToolkit

Used as a reverse-engineering/data-format research reference for original Street Rod concepts including car records, prices, transmissions, tyres, carburettors, manifolds, engines, sprite offsets and garage click regions.

No third-party Toolkit source has been imported into Backyard Racer because a licence was not verified during this pass.

## timoheimonen/amiga-sr2-060-performance-patch

MIT-licensed reverse-engineering and performance work for the Amiga Street Rod 2 executable. It is useful later for original SR2 driving/render timing, road-buffer behaviour and 68k implementation knowledge.

No patch source is currently incorporated into Backyard Racer.

## michelbr84/street-rod-clone

Reviewed as an additional modern implementation reference for screen/state separation around garage, shop, race, repair and economy systems. No source from this project is currently incorporated.

## Infiltratr Common

Backyard Racer consumes `Infiltrator-Projects/Infiltrator-Libraries` as its canonical shared product-neutral dependency. Do not reimplement facilities in Backyard Racer when Common already owns the appropriate implementation.
