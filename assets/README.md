# Backyard Racer asset sources

## Kenney Racing Pack

The drag-strip vehicle sprites integrated into Backyard Racer are derived from Kenney's **Racing Pack**.

- Upstream: https://kenney.nl/assets/racing-pack
- Upstream file used as source: `PNG/Cars/car_blue_1.png`
- License: Creative Commons Zero (CC0 1.0 / public domain dedication)
- Upstream pack contains 420 2D racing assets including vehicles, roads and track objects.

`src/generated_assets.h` contains a compact native X11-oriented representation generated from that source artwork. The blue vehicle keeps the source palette; the red player vehicle is a palette-treated derivative of the same CC0 source. The source sprite is rotated to match Backyard Racer's horizontal drag-strip presentation.

Attribution is not required by CC0, but Kenney is credited here so the project retains clear provenance.

## Kenney Isometric Tiles Vehicles

The menu, classifieds, garage and diner presentation now use real isometric civilian-car art rather than stretching the drag-strip top-down cars into the wrong viewpoint.

- Upstream: https://kenney.nl/assets/isometric-tiles-vehicles
- License: Creative Commons Zero (CC0 1.0 / public domain dedication)
- Reproducible source mirror: `Tiddybub/2d-assets`
- Mirror commit pinned for this import: `e0cbe0d995554a490d4c182fe9beb8769ffbb606`
- Source files:
  - `vehicles/isometric-tiles-vehicles/PNG/Civilian/Red/Sedan 1/carRed2_000.png`
  - `vehicles/isometric-tiles-vehicles/PNG/Civilian/Blue/Sedan 1/carBlue2_000.png`
  - `vehicles/isometric-tiles-vehicles/PNG/Civilian/Red/Sedan 2/carRed3_000.png`
  - `vehicles/isometric-tiles-vehicles/PNG/Civilian/Red/Pick-up 1/carRed1_000.png`

The upstream civilian pack contains Black, Blue, Green, Red and Silver families. `src/generated_isometric_assets.h` stores compact native X11 rectangle data derived from the first sedan sources, `src/generated_vehicle_palette_variants.h` adds CC0 palette derivatives for green, silver and black, and `src/generated_vehicle_body_variants.h` adds a second sedan silhouette and a pickup silhouette from the same CC0 pack.

The classifieds now deliberately mix body shapes and colour families so the newspaper page reads as a varied used-car market instead of duplicated placeholders. `src/main_art_pass.cpp` selects these isometric sprites for presentation screens while retaining the Racing Pack sprites on the actual drag strip, where the top-down viewpoint is appropriate.

## OpenGameArt isometric cars

A further vintage-oriented candidate remains identified for a later visual pass:

- `4 Cars Isometric PixelArt Set` by Yevhen Ishchenko
- https://opengameart.org/content/4-cars-isometric-pixelart-set
- License: CC0

That pack is still worth evaluating for a more explicitly vintage garage look, but it is not claimed as integrated until its individual source sprites are imported and verified.
