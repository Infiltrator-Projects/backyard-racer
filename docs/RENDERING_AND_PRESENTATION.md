# Backyard Racer — Rendering and Presentation Contract

Status: mandatory presentation architecture.

This document defines what the player-facing screen actually is and how Backyard Racer must produce it. It exists to prevent the game from drifting back toward a system-font / primitive-widget / DOS-like presentation while technically remaining in graphics mode.

## Fundamental rule

Backyard Racer is a pixel-graphics program.

Inside the game client area, everything the player sees is graphical content composited into pixel memory by the game. The operating-system/window layer exists only to present the completed game image and deliver input/events.

There is no special visual category called "text mode" inside the game.

`BACKYARD RACER`, `MAIN MENU`, `$2800`, horsepower values, newspaper copy, button labels, icons, gauges, panels and every other visible UI element ultimately become pixels in the same frame buffer as the garage and cars.

Window-manager chrome outside the game client area is not part of this rule; the game-owned client image is.

## Complete-frame software rendering

The game owns complete pixel buffers for presentation.

Conceptually:

```text
front buffer  -> complete frame currently being displayed
back buffer   -> game composes the entire next frame here

compose complete next frame
present/swap buffers

front buffer  -> newly completed frame
back buffer   -> reused for the next complete frame
```

The next frame is composed off-screen before presentation. The player must not watch the game progressively draw individual rectangles, text strings, cars or panels directly onto the visible surface.

Every presented frame is a coherent finished image.

## What is composed into a frame

A frame may contain, in order appropriate to the current scene:

- authored background artwork;
- environmental animated layers;
- vehicle shadows;
- vehicle body/detail/wheel layers;
- particles and effects;
- panels and HUD artwork;
- icons and controls;
- bitmap/graphical glyphs for all visible wording and numbers;
- cursor artwork where the game supplies its own cursor;
- transition/fade/overlay artwork.

The distinction between "world", "UI" and "text" is semantic game logic only. At presentation time they are all pixel layers composited into the same completed frame.

## Text is graphical artwork

System/X11 fonts are not the final presentation system.

A production font is a graphical asset: for example a bitmap-font atlas or sprite sheet containing glyph artwork. Drawing `5` means selecting the graphical glyph for `5` and compositing those pixels into the back buffer.

Likewise, button labels are not system text painted over rectangles. A button is graphical artwork plus graphical glyphs and any state-specific artwork required for normal, hovered, pressed and disabled states.

This rule applies to:

- menus;
- HUD values;
- garage statistics;
- newspaper/classified text;
- race instrumentation;
- dialog and messages;
- settings labels;
- every other in-game word, numeral or symbol.

A font may still be generated procedurally during development or preprocessing, but the runtime presentation result is pixels written into the game-owned frame, not a platform text widget or platform text rendering call layered over the game.

## UI is graphical artwork

The final visual language must not be constructed primarily from calls equivalent to:

```text
draw rectangle
draw line
draw triangle
draw system font glyph
```

Those operations may remain useful internally for debugging, masks, tooling or temporary development aids, but they are not the intended final presentation language.

The production path is conceptually:

```text
blit background artwork
composite animated scene layers
blit/tint vehicle artwork
blit panel artwork
blit button artwork
blit icon artwork
blit graphical glyphs
present completed frame
```

The important distinction is not whether the game is technically in a graphics mode. The important distinction is whether the screen looks authored as graphical art rather than like terminal/UI primitives reproduced inside a framebuffer.

## Garage example

For the garage, one frame is conceptually:

```text
clear/build back buffer
blit permanent open-door garage background
composite current car shadow at current animated X
composite tinted grey body master at current animated X
composite wheels at current signed rotation angles
composite chrome/glass/trim/detail layer
composite garage information panels
composite bitmap-font labels and values
composite buttons/icons
present completed back buffer
```

On the next animation frame, the background may be identical while the car X position and wheel angles differ. The entire screen is still composed as a new complete pixel image before presentation.

The detailed car/garage movement and paint rules are defined in `VEHICLE_PRESENTATION_AND_PAINT.md`.

## Animation and frame production

Simulation state and presentation state are separate concerns.

For example, during a purchased-car arrival:

1. game logic advances the car position and signed distance travelled;
2. wheel angles are derived from that movement;
3. the renderer composes a complete screen using that updated state;
4. only after the complete frame exists is it presented.

The screen is never the source of truth for motion. The game state determines the next frame; the renderer turns that state into pixels.

## Asset-driven composition

Backgrounds, cars, icons, panels, bitmap fonts and other authored presentation elements should be loaded/decoded into reusable graphical resources and cached.

Static artwork must not be decoded again for every pointer movement or animation frame.

Where runtime modification is needed — such as repainting a neutral-grey car body — the renderer may create/cache a derived pixel surface, but the source asset remains independent and reusable.

## Classifieds newspaper contract

The classifieds screen is a special-purpose raster page, not a GUI layout with
newspaper styling applied afterward.

Its production composition happens on a fixed **640x360** low-resolution
newspaper surface. Type, rules, side advertisements, selection marks and the
footer are written into that page as bitmap pixels. The completed page is then
scaled to the game framebuffer with **nearest-neighbour** sampling.

The native page resolution is intentionally high enough that body copy remains
small and dense at a 1280x720 host size. Regressions to 320x180 or other layouts
that enlarge the 5x7 body glyphs into oversized terminal-like lettering are
explicitly rejected. Side-ad text must fit without clipping, body listings must
read as compact classified copy rather than cards, and large full-width black
section banners are not part of the visual language.

## Platform boundary

The platform layer is deliberately thin.

Its responsibilities are approximately:

- create/manage the application window or display surface;
- deliver input and window events;
- provide timing/platform services;
- present the finished frame buffer.

The platform layer must not dictate the visual identity through native widgets, system fonts or ad-hoc platform drawing calls.

This keeps presentation portable: an X11 implementation, SDL implementation or another future backend can present the same game-authored pixel frames.

## Current code versus target architecture

The existing X11 primitive renderer is transitional code. Its current use of platform line/rectangle/string drawing is not the target presentation architecture described here.

It may remain temporarily while the framebuffer compositor and asset pipeline are introduced, but new presentation work should move toward this contract rather than expanding the primitive-drawing approach.

The correct direction is to make the game own the complete visual frame and let X11 merely present it.

## Explicitly rejected approaches

The following violate this contract as a final implementation:

- using X11/system fonts as the production in-game font renderer;
- building the production HUD primarily from raw platform rectangles and lines;
- drawing directly onto the visible window in piecemeal order;
- treating menus or HUD text as a fundamentally different display mode from scene graphics;
- using native OS widgets inside the game presentation;
- baking dynamic UI values into background images;
- presenting half-composed frames;
- allowing the platform backend to define the game's visual language.

## Required result

The player should be able to think of every Backyard Racer frame as one finished image produced by the game.

The garage, car, newspaper, race view, buttons, numbers, labels and icons are all simply different graphical sources that Backyard Racer composites into that image before it is shown.