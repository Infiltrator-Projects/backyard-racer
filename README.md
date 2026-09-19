# Backyard Racer

A native street-rod garage and racing game built around the hands-on car building, classifieds, street racing and pink-slip progression of the classic Street Rod games.

## Engineering ethos

What happens when the hands-on appeal of classic street-rod games is rebuilt from first principles through the underlying car, garage and racing systems rather than cloned as a surface imitation? Backyard Racer owns its vehicle state, economy, progression, race simulation and presentation as one coherent game.

Classic Street Rod games and public engineering work are reference material: they show mechanics worth understanding, but they do not define this project's runtime behaviour. Reusable first-party infrastructure comes from an exact pinned Common revision; game-specific rules remain here. External code is absorbed only deliberately, with its licence and behaviour understood, rather than becoming an uncontrolled dependency whose later changes can alter the game.

The project keeps proven ideas when they remain fun and mechanically sound, and replaces them when a new design measurably improves control, fidelity, readability or maintainability. The aim is not novelty for its own sake, but a stronger implementation of the experience.

## Shared foundation

Backyard Racer consumes **Infiltratr Common** as the canonical shared-code dependency instead of maintaining private copies of reusable project infrastructure.

The build is pinned to Infiltratr Common **v1.19.7**, exact release commit:

`882c61a1e8626d4c733bdf45bda7152a30e56ded`

The current game uses Common for canonical project/application identity, the exact fixed-step scheduler used by the live quarter-mile race, monotonic host timing, durable atomic save publication, software surfaces, alpha composition, luminance tinting, nearest/bilinear image scaling and locale-independent fixed-point formatting for race timing. Pixel-art screens such as the low-resolution newspaper deliberately retain nearest-neighbour scaling; photographic/authored UI and vehicle assets use Common's alpha-aware bilinear path.

Common 1.19.7 also publishes shared application theme, design-metric and MB Corpo typography contracts. Backyard Racer deliberately does **not** apply those contracts to the in-game presentation: its garage, newspaper, race HUD and bitmap type are authored game art with their own visual language. Shared mechanics come from Common where they are stronger; game-specific presentation remains here.

## Presentation architecture

Backyard Racer's final presentation is asset-driven and full-frame: the game owns the complete pixel image, composes the entire next frame off-screen, then presents that completed frame. Backgrounds, cars, UI panels, icons, labels and numbers are all graphical pixel content; system-font and primitive X11 drawing is transitional rather than the target visual architecture.

The mandatory rendering contract is documented in [`docs/RENDERING_AND_PRESENTATION.md`](docs/RENDERING_AND_PRESENTATION.md).

Garage/car presentation, including the single persistent open-door garage, neutral-grey recolourable car masters, historically plausible acquisition colours, 24-bit repainting, purchase drive-in animation, reverse-out/forward-in car swapping and distance-driven wheel rotation, is documented in [`docs/VEHICLE_PRESENTATION_AND_PAINT.md`](docs/VEHICLE_PRESENTATION_AND_PAINT.md).

## Current playable loop

Version `0.6.0-dev` has a basic garage-to-race loop and persistent progression.

The current loop is:

`New Game -> Classifieds -> Read Ad -> Buy Car -> Garage -> Build/Tune -> Diner -> Cash or Pink-Slip Race -> Garage`

Implemented gameplay includes:

- rotating used-car classifieds presented as a newspaper, with a full article opened before any purchase action;
- model-specific HD side-view art for every current body family, generated from supersampled layered masters rather than upscaled sprites;
- multiple owned cars and active-car selection;
- independently removable/refittable front and rear bumpers, with paid garage labour and persistent fitment state;
- cash economy;
- carburettor, intake, exhaust, camshaft, transmission, tyre and full engine upgrades;
- engine swaps that materially change horsepower and survive saves;
- upgrades affecting horsepower, traction and shift performance;
- replaced parts retained in the player's parts bin;
- diner opponents and reputation ladder;
- live player-controlled quarter-mile racing with throttle, manual shifts, RPM and distance;
- $100 and $250 cash wagers;
- pink-slip races where winning adds the opponent car to the garage and losing removes the player's car;
- race wear, repair cost and resale value;
- automatic persistent saves including garage, active car, installed parts, spare parts, cash, reputation, record and classifieds state;
- Continue across process restarts;
- gameplay save/load, engine-upgrade and race-session smoke tests in CI;
- a native X11 presentation backend, currently being migrated from primitive drawing toward the full-frame asset compositor defined above.

Save data follows XDG conventions where available and defaults to `~/.local/share/backyard-racer/save_v1.txt` on a typical Linux desktop. `BACKYARD_RACER_SAVE` can override the path for testing or portable setups.

The garage mechanics are intentionally modular so the quick auto-install flow can later gain the classic hands-on nut/bolt interaction without replacing the underlying car/part state model.

## Art sources

The active garage/newspaper vehicle presentation uses a complete first-party model-specific HD neutral-grey master roster. Cars are built from supersampled paint, detail, wheel and removable-bumper layers; the obsolete 240×80 vehicle sprite path has been removed. Older Kenney CC0 material remains only as legacy/prototype provenance. See [`assets/README.md`](assets/README.md) for the active roster and tracked third-party history.

## Upstream acceleration

Rather than redesigning known Street Rod mechanics from scratch, development uses public Street Rod-related engineering work as reference material where appropriate. See [`docs/UPSTREAM.md`](docs/UPSTREAM.md) for the current inventory and licence treatment.

The GPL-2.0-or-later **StreetRod3Classic** source provides useful garage/newspaper/parts/player/opponent/racing architectural reference that can be ported into this GPL-3.0-or-later project without inheriting its obsolete SDL/OpenGL platform layer wholesale.

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
