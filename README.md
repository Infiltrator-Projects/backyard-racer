# Backyard Racer

A native street-rod garage and racing game built around the hands-on car building, classifieds, street racing and pink-slip progression of the classic Street Rod games.

## Shared foundation

Backyard Racer consumes **Infiltratr Common** as the canonical shared-code dependency instead of maintaining private copies of reusable project infrastructure.

The build is pinned to Infiltratr Common **v1.16.0**, exact release commit:

`a9db06b11f493c4e6f42bf6c13cc5cc5c73e1fc4`

The current game uses Common for canonical project/application identity, string handling used by the command-line interface, and the fixed-step scheduler used by the live quarter-mile race. Product-neutral functionality already owned by Common should continue to be consumed from Common rather than recreated here.

## Current playable loop

Version `0.6.0-dev` now has a complete basic garage-to-race loop, persistent progression and a real CC0 art pass.

The current loop is:

`New Game -> Classifieds -> Buy Car -> Garage -> Buy/Install Parts -> Diner -> Cash or Pink-Slip Race -> Garage`

Implemented now:

- rotating used-car classifieds;
- mixed CC0 isometric used-car artwork with multiple body shapes and colour families instead of programmer rectangles/top-down cars on presentation screens;
- multiple owned cars and active-car selection;
- cash economy;
- carburettor, intake, exhaust, camshaft, transmission, tyre and full engine upgrades;
- engine swaps that materially change horsepower and are retained through saves just like the smaller bolt-on parts;
- upgrades that alter horsepower, traction and shift performance;
- replaced parts retained in the player's parts bin;
- Street Rod-style diner opponents and reputation ladder;
- live player-controlled quarter-mile race with throttle, manual shifts, RPM and distance;
- $100 and $250 cash wagers;
- pink-slip races where winning adds the opponent car to your garage and losing removes yours;
- race wear, repair cost and resale value;
- automatic persistent saves, including garage, active car, installed parts, spare parts, cash, reputation, record and classifieds state;
- Continue now survives quitting and reopening the game;
- gameplay save/load, engine-upgrade and race-session smoke tests in CI;
- native X11 double-buffered interface.

Save data follows XDG conventions where available and defaults to `~/.local/share/backyard-racer/save_v1.txt` on a typical Linux desktop. `BACKYARD_RACER_SAVE` can override the path for testing or portable setups.

The garage mechanics are intentionally modular so the quick auto-install flow can later gain the classic hands-on nut/bolt interaction without replacing the underlying car/part state model.

## Art sources

The drag strip uses Kenney's CC0 Racing Pack while the menu, classifieds, garage and diner use Kenney's CC0 Isometric Tiles Vehicles art. The classifieds intentionally mix red, blue, green, silver and black variants plus multiple sedan/pickup silhouettes from the same pack. See [`assets/README.md`](assets/README.md) for exact provenance and the pinned reproducible mirror commit.

## Upstream acceleration

Rather than redesigning known Street Rod mechanics from scratch, development is using public Street Rod-related engineering work as reference material where appropriate. See [`docs/UPSTREAM.md`](docs/UPSTREAM.md) for the current inventory and licence treatment.

Most importantly, the GPL-2.0-or-later **StreetRod3Classic** source provides proven garage/newspaper/parts/player/opponent/racing architecture that can be ported into this GPL-3.0-or-later project without inheriting its obsolete SDL/OpenGL platform layer wholesale.

## Build on Linux

Requirements:

- CMake 3.20+
- C/C++ compiler with C11/C++20 support
- X11 development headers/libraries
- Git/network access on the first configure so CMake can fetch the pinned Common revision

```sh
make
make run
```

The executable is created at `build/backyard-racer`.

Useful checks:

```sh
ctest --test-dir build --output-on-failure
./build/backyard-racer --version
./build/backyard-racer --project-info
```
