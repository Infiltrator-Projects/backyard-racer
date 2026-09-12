# Backyard Racer

A native street-rod garage and racing game built around the hands-on car building, classifieds, street racing and pink-slip progression of the classic Street Rod games.

## Shared foundation

Backyard Racer consumes **Infiltratr Common** as the canonical shared-code dependency instead of maintaining private copies of reusable project infrastructure.

The build is pinned to Infiltratr Common **v1.15.7**, exact release commit:

`e9b747c7c0530590fbba1f57788e046203ef8c4e`

The current game uses Common for canonical project/application identity, string handling used by the command-line interface, and checked framebuffer/XImage allocation arithmetic. Product-neutral functionality already owned by Common should continue to be consumed from Common rather than recreated here.

## Current playable loop

Version `0.2.0-dev` moves the project beyond the menu/garage placeholder. A new game now starts with cash and opens the used-car classifieds.

The current loop is:

`New Game -> Classifieds -> Buy Car -> Garage -> Buy/Install Parts -> Diner -> Cash or Pink-Slip Race -> Garage`

Implemented now:

- rotating used-car classifieds;
- multiple owned cars and active-car selection;
- cash economy;
- carburettor, intake, exhaust, camshaft, transmission and tyre upgrades;
- upgrades that alter horsepower, traction and shift performance;
- Street Rod-style diner opponents;
- quarter-mile result simulation based on power-to-weight, traction, shifting and reaction time;
- $100 and $250 cash wagers;
- pink-slip races where winning adds the opponent car to your garage and losing removes yours;
- gameplay-core smoke tests in CI;
- native X11 interface and the existing New Game / Continue / Settings / Quit shell.

The garage mechanics are intentionally modular so the quick auto-install flow can later gain the classic hands-on nut/bolt interaction without replacing the underlying car/part state model.

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
