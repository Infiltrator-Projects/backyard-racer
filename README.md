# Backyard Racer

A native street-racing and car-building game inspired by the hands-on garage loop of classic Street Rod-era games.

The project intentionally reuses the proven native menu/window shell from the companion `Infiltrator-Projects/Egypt` project so common engine/UI code is not rewritten unnecessarily.

## Current milestone

- Native X11 window
- Reused framebuffer, bitmap-font, input, resize and menu-button systems from Egypt
- Main menu: New Game / Continue / Settings / Quit
- New Game opens the first garage placeholder
- Escape returns to the menu; Escape from the menu quits
- Continue remains disabled until save-game support exists

## Build on Linux

Dependencies: a C++20 compiler, `make`, and X11 development headers/libraries.

```bash
make
make run
```

The executable is written to `build/backyard-racer`.

## Direction

The intended core loop is classifieds -> buy a car -> garage work -> parts/tuning -> street meet -> race for cash or pink slips -> improve the car and reputation.
