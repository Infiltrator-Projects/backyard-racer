# Architecture

## Purpose

Backyard Racer is a native street-rod garage and racing game built around hands-on car building, classifieds, tuning, racing and persistent progression.

## System decomposition

- C++ game core
- garage/economy/progression state
- quarter-mile race simulation
- asset-driven presentation
- native X11 platform/presentation layer
- Common graphics/timing/persistence primitives
- gameplay/race/pixel/paint regression tests

## Ownership boundaries

Classic Street Rod games and public engineering work are reference material, not runtime specifications. Backyard Racer owns game rules, progression, car state and presentation; Common owns only product-neutral primitives.

Platform APIs, reference implementations and first-party shared libraries are mechanisms or evidence behind explicit boundaries. They must not silently redefine product behaviour.

## Source of truth

Code and tests define executable behaviour. Generated output is authoritative only where the build explicitly defines it as an artifact; editable source remains the owning source of truth.

## Change discipline

Keep game/site/calculation domain logic independent from rendering/toolkit/hosting details where practical. Reuse shared first-party primitives without moving product-specific policy into Common.

## Specialist documentation

- docs/RENDERING_AND_PRESENTATION.md
- docs/VEHICLE_PRESENTATION_AND_PAINT.md
- docs/UPSTREAM.md
