# Backyard Racer asset sources

## Kenney Racing Pack

The first real vehicle sprites integrated into Backyard Racer are derived from Kenney's **Racing Pack**.

- Upstream: https://kenney.nl/assets/racing-pack
- Upstream file used as source: `PNG/Cars/car_blue_1.png`
- License: Creative Commons Zero (CC0 1.0 / public domain dedication)
- Upstream pack contains 420 2D racing assets including vehicles, roads and track objects.

`src/generated_assets.h` contains a compact native X11-oriented representation generated from that source artwork. The blue vehicle keeps the source palette; the red player vehicle is a palette-treated derivative of the same CC0 source. The source sprite is rotated to match Backyard Racer's horizontal drag-strip presentation.

Attribution is not required by CC0, but Kenney is credited here so the project retains clear provenance.

## OpenGameArt isometric cars

Candidate garage/classifieds artwork identified for the next visual pass:

- `4 Cars Isometric PixelArt Set` by Yevhen Ishchenko
- https://opengameart.org/content/4-cars-isometric-pixelart-set
- License: CC0

This pack has a more appropriate vintage/isometric view for the Street Rod-style garage than the top-down Kenney race cars. It should be used for garage/classifieds once individual source sprites are imported cleanly rather than stretching the top-down race art into the wrong viewpoint.
