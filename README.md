# Backyard Racer

A native street-rod garage and racing game inspired by the hands-on car building, classifieds, street racing and pink-slip progression of the classic Street Rod games.

## Shared foundation

Backyard Racer consumes **Infiltratr Common** as the canonical shared-code dependency instead of maintaining private copies of reusable project infrastructure.

The build is pinned to Infiltratr Common **v1.15.7**, exact release commit:

`e9b747c7c0530590fbba1f57788e046203ef8c4e`

The current game uses Common for:

- canonical `InfiltratrProjectInfo` application identity;
- Common string comparison for command-line handling;
- checked `size_t` multiplication for framebuffer storage;
- checked `size_t` multiplication for XImage storage.

As the game grows, additional product-neutral mechanics already owned by Common (parsing, UTF-8 validation, timing/cadence, localisation, formatting and generic POSIX helpers) should be consumed from Common when the game actually needs them rather than reimplemented locally.

Game-specific rendering, X11 input/window integration, menu layout, garage state, cars, economy and racing remain Backyard Racer-owned.

## Current milestone

- Native X11 window and framebuffer renderer
- Main menu: New Game / Continue / Settings / Quit
- New Game opens the first garage placeholder
- Escape returns to the menu; Escape from the menu quits
- Continue remains disabled until save-game support exists

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

Useful metadata commands:

```sh
./build/backyard-racer --version
./build/backyard-racer --project-info
```

## Direction

The intended core loop is classifieds -> buy a car -> garage work -> parts/tuning -> street meet -> race for cash or pink slips -> improve the car and reputation.
