# Backyard Racer asset sources

## Active model-specific vehicle masters

The active garage and newspaper renderer uses the approved first-party, model-specific **authored side-view car artwork**. The former procedural body generator and the old 240×80 sprite path are not used by the active renderer.

The complete current body roster is:

- 1964 Ford Falcon
- 1965 Ford Mustang
- 1966 Chevrolet Nova
- 1967 Chevrolet Camaro
- 1968 Dodge Charger
- 1969 Pontiac GTO
- 1969 Plymouth Road Runner
- 1970 Dodge Challenger
- 1968 Mercury Cougar
- 1969 Ford Torino
- 1970 Chevrolet Chevelle
- 1970 Plymouth Barracuda

Each body ID maps to its own authored car image in the packed vehicle-art sheet. The runtime expands that artwork into cached presentation surfaces and separates paintable bodywork, fixed details, wheels and bumpers for the game mechanics. Drivetrain variants sharing the same body ID reuse the same body artwork, while the runtime vehicle state supplies the specific engine/variant data.

All active masters use the garage's canonical side orientation: **front on the left, rear on the right**. Cars enter from the right edge and travel right-to-left in forward motion; leaving cars reverse left-to-right without flipping the artwork.

The authored cars are side-on presentation artwork. The renderer segments each approved image into paint, fixed-detail, front-wheel, rear-wheel, front-bumper and rear-bumper surfaces, preserving the actual model image instead of replacing it with a generated geometric approximation. Body paint is tinted at runtime; glass, lamps, grille, trim, tyres and chrome remain fixed. Front and rear bumpers can therefore be removed and refitted independently without regenerating the vehicle image.

## Active garage and UI artwork

The permanent garage scene, UI font atlas and menu chrome remain embedded first-party presentation assets. The garage background never contains the selected car.

## Legacy Kenney imports

Older prototypes used Kenney Racing Pack and Kenney Isometric Tiles Vehicles assets under CC0. Those files and generated headers may remain in the repository for historical/provenance purposes, but they are not the active named-car presentation path.

- Racing Pack: https://kenney.nl/assets/racing-pack
- Isometric Tiles Vehicles: https://kenney.nl/assets/isometric-tiles-vehicles
- License: CC0 1.0 / public-domain dedication

The active vehicle renderer must not fall back to those generic silhouettes for a named historical model.
