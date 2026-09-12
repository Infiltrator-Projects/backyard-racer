# Backyard Racer — Vehicle Presentation and Paint Architecture

Status: design contract for the garage/car presentation layer.

This document defines the visual architecture for the garage and vehicle assets before further presentation work is implemented. The purpose is to prevent the garage, car art, animation and paint system from being designed independently and then fighting each other later.

## Core visual rule

The garage is one persistent authored background scene. The selected car is never baked into the garage artwork.

Changing cars changes only the vehicle layers and vehicle state. It must not replace or redraw a different garage scene.

The garage door is open. Whenever a car is presented in the garage, the car enters from off-screen on the right and travels right-to-left into its parked presentation position. There is no visible driver. The wheels rotate according to vehicle travel while the car is moving and stop when the vehicle reaches its parked position.

This arrival behaviour applies to every car model.

## Garage background

The garage background is a single reusable scene asset. It should be visually close to a real working backyard/hot-rod garage rather than a collection of procedural rectangles.

The background owns the environment only: walls, open garage door, ceiling, lights, shelves, benches, tools, cabinets, floor, stains, shadows belonging to fixed objects, and other workshop dressing.

It must not contain a vehicle, vehicle shadow, vehicle-specific signage, vehicle paint colour, or anything else that would make the background specific to the currently selected car.

The same garage background remains visible when the player changes from one car to another.

## Vehicle asset principle: neutral grey masters

Every vehicle model is authored from a neutral grey master for the paintable bodywork.

The neutral grey is not the final in-game colour. It is a luminance/shading source. The renderer tints the grey body at runtime so the same source asset can represent any paint colour while retaining highlights, reflections, panel shading and body contours.

Only paintable body surfaces are recoloured. Tyres, glass, chrome, lights, badges, trim, grilles, exhausts and other non-painted details must remain independent of the chosen paint colour.

A model therefore uses separate logical layers even if the authoring pipeline exports them together:

- shadow layer;
- paintable neutral-grey body layer;
- fixed-detail layer for chrome, glass, lamps, badges, trim and non-painted surfaces;
- independently rotatable front wheel;
- independently rotatable rear wheel.

The source/master artwork for each model should make the paintable shell grey. It should not permanently bake red, cream, blue or any other paint colour into the vehicle.

## Model fidelity

A car ID maps to artwork for that actual model. A text label is not sufficient.

For example, `falcon64` must visibly be a 1964 Ford Falcon; `mustang65` must visibly be a 1965 Ford Mustang; `charger68` must visibly be a 1968 Dodge Charger; and so on.

Each model asset must preserve the recognisable real-car proportions and major visual cues: roofline, greenhouse, bonnet and boot proportions, wheelbase, front and rear overhang, grille/headlamp treatment, window shape, stance, trim and model-specific body character.

The model-art registry is keyed by `CarSpec::id`. Missing artwork is an error/fallback condition, not permission to silently show another model's body.

## Factory colour on acquisition

When a car first enters the player's world, it receives a paint colour selected from a historically plausible factory-colour list for that model/year.

The random selection is from actual colours that the real vehicle could have been sold in, not an arbitrary RGB colour. White, where historically offered, is simply one of those possible factory colours.

The selected colour becomes part of the owned car's persistent state and is saved with the car. Reloading the game must not randomly choose a new colour.

Recommended persisted paint state:

- 24-bit RGB value;
- factory colour name when applicable;
- whether the current colour is a factory colour or a custom repaint.

The factory-colour catalogue belongs to vehicle data, not to the renderer.

## Player paint system

The garage/paint feature will allow the player to repaint a vehicle using a full 24-bit RGB colour chooser: 256 red values × 256 green values × 256 blue values = 16,777,216 selectable colours.

Choosing a custom colour changes the stored vehicle paint RGB and immediately recolours the neutral-grey body layer. It does not require another car image to be generated or stored.

The colour transformation must preserve the original grey master's luminance so that shadows and highlights survive repainting. Near-white paint should still show panel shading and reflections; near-black paint should retain enough lifted highlights to preserve body form.

The renderer must never tint tyres, windows, chrome, lamps or trim when paint colour changes.

## Runtime compositing order

The intended garage composition is:

1. persistent garage background;
2. vehicle ground/contact shadow;
3. tinted neutral-grey body;
4. rotating front and rear wheels;
5. fixed vehicle details such as chrome, glass, lights, badges and trim;
6. game UI and interaction overlays.

The vehicle's complete visual object moves as one unit during garage entry, while wheel rotation is animated independently.

## Garage arrival animation

On entering the garage with a selected car, and when presenting a newly selected car:

1. show the unchanged garage background with the door open;
2. place the selected vehicle fully off-screen to the right;
3. animate the vehicle toward the parked X position;
4. rotate both wheels according to linear distance travelled and effective wheel radius;
5. show no driver inside the vehicle;
6. decelerate naturally into the final parked position;
7. stop wheel rotation when linear movement reaches zero;
8. leave the vehicle parked as an independent foreground layer.

Wheel angle should be derived from distance travelled, not from an arbitrary animation timer, so the wheels cannot visibly slide across the floor.

A simple physical relationship is sufficient:

`wheel_angle += distance_travelled / wheel_radius`

with the result converted to the renderer's angular unit.

## Car switching

Selecting another owned car must not alter the garage background. The display is cleared of the previous vehicle and the newly selected vehicle performs the same right-to-left garage-entry presentation.

Whether the old vehicle later receives an explicit drive-out animation can be decided separately; it is not required by this contract.

## Asset layout

A practical repository layout is:

```text
assets/
  garage/
    garage_open.png
  cars/
    falcon64/
      body_grey.png
      details.png
      wheel_front.png
      wheel_rear.png
      shadow.png
    mustang65/
      body_grey.png
      details.png
      wheel_front.png
      wheel_rear.png
      shadow.png
    ...
```

Equivalent packed formats are acceptable later, but the logical separation must remain because body recolouring and wheel rotation require independent layers.

## Renderer requirements

The presentation renderer must support:

- alpha-transparent PNG assets;
- scaled compositing without destroying alpha;
- tinting a neutral-grey body layer with an arbitrary 24-bit RGB colour while preserving luminance;
- independent wheel rotation;
- per-model asset lookup by car ID;
- vehicle translation for garage-entry animation;
- caching decoded/scaled assets so normal pointer motion does not repeatedly decode images or rebuild static scene data.

The garage background should be decoded once and cached. Car model assets should likewise be cached after first use.

## Non-goals / explicitly rejected approaches

The following are not acceptable as the final architecture:

- baking a specific car into the garage background;
- using a different garage image for each vehicle;
- permanently baking a car's paint colour into the model artwork;
- using one generic car silhouette for multiple named real models;
- repainting chrome, glass, tyres or trim with the body colour;
- showing a static composed screenshot instead of independently rendered scene and vehicle layers;
- generating a fresh raster image every time the player chooses a paint colour;
- animating wheels independently of actual distance travelled.

## Current design decision

The neutral-grey-master approach is the preferred design. It gives Backyard Racer one accurate model asset per real car, historically plausible factory colours when cars are acquired, unrestricted 16.7-million-colour repainting later, and a single persistent realistic garage scene in which every vehicle can arrive, move, park and be repainted independently.
