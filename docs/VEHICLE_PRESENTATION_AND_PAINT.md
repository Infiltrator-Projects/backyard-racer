# Backyard Racer — Vehicle Presentation and Paint Architecture

Status: design contract for the garage/car presentation layer.

This document defines the visual architecture for the garage and vehicle assets. The purpose is to stop the garage, car art, animation and paint system from being designed independently and then fighting each other later.

## Core visual rule

The garage is one persistent authored background scene. The selected car is never baked into the garage artwork.

Changing cars changes only the vehicle layers and vehicle state. It must never substitute a different garage image.

The garage door is permanently open for the vehicle-presentation sequence. Whenever a car is presented in the garage, the game itself animates that independent vehicle object; movement is never pre-rendered into a composed screenshot or baked into the garage background.

There is no visible driver.

## Garage background

The garage background is a single reusable scene asset and should look close to a real working backyard/hot-rod garage rather than a collection of procedural rectangles.

The background owns the environment only: walls, the open garage door, ceiling, lights, shelves, benches, tools, cabinets, floor, stains, fixed-object shadows and other workshop dressing.

It must not contain a vehicle, vehicle shadow, vehicle-specific paint colour or anything that makes the background specific to the currently selected car.

The exact same garage background remains visible when a car is purchased, when a car arrives, while it is parked, while it leaves, while another car arrives and while the garage is temporarily empty between cars.

## Vehicle asset principle: neutral grey masters

Every vehicle model is authored from a neutral-grey master for the paintable bodywork.

The neutral grey is not the final in-game colour. It is a luminance/shading source. The renderer tints the grey body at runtime so the same source asset can represent any paint colour while retaining highlights, reflections, panel shading and body contours.

Only paintable body surfaces are recoloured. Tyres, glass, chrome, lights, badges, trim, grilles, exhausts and other non-painted details remain independent of the chosen paint colour.

A model therefore uses separate logical layers even if the authoring pipeline later packs some of them together:

- shadow layer;
- paintable neutral-grey body layer;
- fixed-detail layer for chrome, glass, lamps, badges, trim and non-painted surfaces;
- independently rotatable front wheel;
- independently rotatable rear wheel.

The source/master artwork for each model must not permanently bake red, cream, blue or any other body colour into the paintable shell.

## Model fidelity

A car ID maps to artwork for that actual model. A text label is not sufficient.

For example, `falcon64` must visibly be a 1964 Ford Falcon; `mustang65` must visibly be a 1965 Ford Mustang; `charger68` must visibly be a 1968 Dodge Charger; and so on.

Each model asset must preserve the recognisable real-car proportions and major visual cues: roofline, greenhouse, bonnet and boot proportions, wheelbase, front and rear overhang, grille/headlamp treatment, window shape, stance, trim and model-specific body character.

The model-art registry is keyed by `CarSpec::id`. Missing artwork is an error/fallback condition, not permission to silently show another model's body.

## Factory colour on acquisition

When a car first enters the player's world, it receives a paint colour selected from a historically plausible factory-colour list for that model/year.

The random selection is from real colours that vehicle could have been sold in, not an arbitrary RGB colour. White, where historically offered, is simply one possible factory colour.

The selected colour becomes part of the individual owned car's persistent state and is saved with the car. Reloading the game must not randomly choose a new colour.

Persisted paint state should contain:

- 24-bit RGB value;
- factory colour name when applicable;
- whether the current colour is factory or custom.

The factory-colour catalogue belongs to vehicle data, not to the renderer.

## Player paint system

The player can later repaint a vehicle using a full 24-bit RGB colour chooser: 256 red values × 256 green values × 256 blue values = 16,777,216 selectable colours.

Choosing a custom colour changes the stored vehicle paint RGB and immediately recolours the neutral-grey body layer. It does not require another car image to be generated or stored.

The colour transformation must preserve the grey master's luminance so shadows, highlights, reflections and panel contours survive repainting. Near-white paint must still show body shading; near-black paint must retain enough highlights to preserve form.

The renderer must never tint tyres, windows, chrome, lamps or trim when body paint changes.

## Buying a car from the newspaper

Buying a car from Classifieds/Newspaper has a mandatory in-game presentation sequence:

1. complete the purchase and create the owned-car state, including its persistent factory paint colour;
2. transition to the garage while keeping the one permanent garage background;
3. initially show the garage with no car parked;
4. place the purchased car completely off-screen to the right;
5. the running game advances the car's X position frame by frame from right to left;
6. the wheels rotate forward according to the actual distance travelled;
7. there is no driver visible;
8. the car decelerates into the fixed parked position;
9. wheel rotation stops exactly when the car stops;
10. garage controls become fully active with the new car parked.

This is runtime game animation. It must not be represented by a pre-drawn image of a car already sitting in the garage.

## Runtime garage compositing order

For each complete rendered frame, the intended garage composition is:

1. permanent garage background;
2. vehicle ground/contact shadow at the current vehicle position;
3. tinted neutral-grey body at the current vehicle position;
4. independently rotated front and rear wheels;
5. fixed vehicle details such as chrome, glass, lights, badges and trim;
6. game UI and interaction artwork.

The complete vehicle visual object translates as one unit; wheel angle changes independently according to travel.

## Arrival motion

For a vehicle entering the garage:

1. start fully off-screen to the right;
2. travel right-to-left toward the fixed parked X position;
3. rotate both wheels according to linear distance travelled and effective wheel radius;
4. decelerate naturally into the parked position;
5. stop wheel rotation when linear movement reaches zero.

Wheel angle is derived from distance travelled rather than an arbitrary looping timer so the tyres cannot visibly slide across the floor.

Conceptually:

`wheel_angle += distance_travelled / wheel_radius`

with direction preserved. A negative travel delta reverses wheel rotation.

## Switching owned cars

Switching cars is also a mandatory runtime animation and uses the same permanent garage background.

The sequence is:

1. current car begins at the parked position;
2. current car reverses from left to right toward the open garage door;
3. its wheels rotate backwards according to actual reverse distance travelled;
4. current car continues until completely off-screen to the right;
5. for a short transition the same garage is visible empty;
6. `GameState` selects the next owned car;
7. the next car begins completely off-screen to the right;
8. the next car drives right-to-left into the same parked position;
9. its wheels rotate forwards according to actual distance travelled;
10. it decelerates, stops, and its wheels stop with it.

The car does not flip direction just to leave. The visual model remains in its normal side orientation; leaving to the right is a reversing motion and entering from the right is forward motion. This is the intended "roll backwards / roll forwards" presentation.

Input that would mutate the active garage car should be locked or deliberately queued while a swap animation is in progress so the scene cannot end in an impossible mixed state.

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

- alpha-transparent vehicle assets;
- scaled compositing without destroying alpha;
- tinting a neutral-grey body layer with arbitrary 24-bit RGB while preserving luminance;
- independent wheel rotation;
- per-model asset lookup by car ID;
- vehicle translation controlled by runtime game state;
- forward and reverse wheel rotation derived from signed travel distance;
- decoded/scaled asset caching so animation does not repeatedly decode source images;
- the complete-frame presentation contract in `RENDERING_AND_PRESENTATION.md`.

The garage background should be decoded once and cached. Car model assets should likewise be cached after first use.

## Explicitly rejected approaches

The following are not acceptable as the final architecture:

- baking a specific car into the garage background;
- using a different garage image for each vehicle;
- permanently baking a car's paint colour into the model artwork;
- using one generic silhouette for multiple named real models;
- repainting chrome, glass, tyres or trim with body colour;
- showing a static composed screenshot instead of independent scene and vehicle layers;
- pre-rendering the drive-in or drive-out animation into images/video;
- instantly replacing one parked car with another without the reverse-out / drive-in sequence;
- generating a fresh raster source asset every time the player selects a paint colour;
- animating wheels independently of actual distance travelled.

## Current design decision

The neutral-grey-master approach is the required design. It gives Backyard Racer one accurate model asset per real car, historically plausible factory colours on acquisition, unrestricted 16.7-million-colour repainting, and one permanent realistic garage scene in which every vehicle can arrive, park, reverse out, be replaced and be repainted independently.