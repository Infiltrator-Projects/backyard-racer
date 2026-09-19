# Backyard Racer — Vehicle Presentation and Paint Architecture

Status: design contract for the garage/car presentation layer.

This document defines the visual architecture for the garage and vehicle assets. The purpose is to stop the garage, car art, animation and paint system from being designed independently and then fighting each other later.

## Core visual rule

The garage is one persistent authored background scene. The selected car is never baked into the garage artwork.

Changing cars changes only the vehicle layers and vehicle state. It must never substitute a different garage image.

The garage door is open for vehicle entry/exit. Whenever a car changes the physical occupancy of the garage, the game itself animates that independent vehicle object; movement is never pre-rendered into a composed screenshot or baked into the garage background.

There is no visible driver.

The key state is **which car is physically present in the garage**, not merely which car is selected in data. A car must not repeatedly drive in just because the garage screen is reopened while that same car is already parked there.

## Garage background

The garage background is a single reusable scene asset and should look close to a real working backyard/hot-rod garage rather than a collection of procedural rectangles.

The background owns the environment only: walls, the open garage door, ceiling, lights, shelves, benches, tools, cabinets, floor, stains, fixed-object shadows and other workshop dressing.

It must not contain a vehicle, vehicle shadow, vehicle-specific paint colour or anything that makes the background specific to the currently presented car.

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
- independently rotatable rear wheel;
- independently removable front-bumper layer;
- independently removable rear-bumper layer.

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

## Garage occupancy state

The garage presentation has an explicit physical occupancy state:

- **empty garage** — no vehicle is currently parked in the scene;
- **occupied garage** — one specific owned vehicle is physically parked in the scene;
- **transitioning** — a vehicle is driving out or driving in.

Only one car can physically occupy the garage presentation at a time.

Opening or returning to the garage does not automatically trigger an arrival animation. If the same car is already physically parked there, it remains parked.

A drive-in/drive-out sequence is triggered only when garage occupancy changes.

## Reading and buying from the newspaper

Clicking a classified listing opens that listing's full newspaper article. A listing click never purchases a vehicle directly.

The article is still part of the newspaper presentation and shows the model-specific vehicle image, asking price, engine/variant, metric power and mass, seller notes and the classified reference. Purchase is a separate explicit action inside the article. Returning from the article does not alter the listing or the player's money.

After the explicit purchase action, buying from Classifieds/Newspaper behaves differently depending on whether the garage is currently empty or occupied.

### First car / empty garage

If there is no car currently occupying the garage:

1. complete the purchase and create the owned-car state, including its persistent factory paint colour;
2. transition to the unchanged empty garage background;
3. place the purchased car completely off-screen to the right;
4. the running game advances the car's X position frame by frame from right to left;
5. the wheels rotate forwards according to actual distance travelled;
6. there is no driver visible;
7. the car decelerates into the fixed parked position;
8. wheel rotation stops exactly when the car stops;
9. the garage occupancy becomes that car and normal garage controls become active.

There is no outgoing-car animation because the garage was empty.

### Buying another car while one is already parked

If a car already physically occupies the garage when another car is purchased:

1. complete the purchase and create the new owned-car state, including its persistent factory paint colour;
2. keep the exact same garage background on screen;
3. the currently parked car reverses from left to right toward the open garage door;
4. its wheels rotate backwards according to actual reverse distance travelled;
5. it continues until completely off-screen to the right;
6. the garage is briefly empty;
7. the newly purchased car begins completely off-screen to the right;
8. it drives forwards from right to left into the fixed parked position;
9. its wheels rotate forwards according to actual distance travelled;
10. it decelerates and stops;
11. garage occupancy changes to the newly purchased car and normal controls become active.

The old car remains owned; it has merely left the physical garage presentation so the new car can enter.

This entire sequence is runtime game animation. It must not be represented by a pre-drawn image or video containing both cars or a car already sitting in the garage.

## Moving/switching another owned car into the garage

If the player asks to bring a different already-owned car into the garage while another car is physically parked there, the same swap rule applies:

1. current parked car reverses left-to-right out through the open door;
2. wheels rotate backwards from signed travel distance;
3. current car becomes fully off-screen;
4. the garage is briefly empty;
5. the requested car begins fully off-screen right;
6. requested car drives right-to-left into the same parked position;
7. wheels rotate forwards from signed travel distance;
8. requested car decelerates and stops;
9. garage occupancy becomes the requested car.

If the garage is empty, only the incoming half of that sequence occurs.

If the requested car is already physically parked in the garage, no movement occurs.

The visual model does not flip direction merely to reverse. Every active side-view vehicle master is permanently **left-facing**: the front of the car is on the left and the rear is on the right. Right-to-left movement is therefore forward motion into the garage; left-to-right movement is reverse motion out of the garage.

## Runtime garage compositing order

For each complete rendered frame, the intended garage composition is:

1. permanent garage background;
2. vehicle ground/contact shadow at the current vehicle position, if a vehicle is visible;
3. tinted neutral-grey body at the current vehicle position;
4. independently rotated front and rear wheels;
5. fixed vehicle details such as chrome, glass, lights, badges and trim;
6. front bumper, if currently fitted;
7. rear bumper, if currently fitted;
8. game UI and interaction artwork.

The complete vehicle visual object translates as one unit; wheel angle changes independently according to travel.

## Vehicle motion

For a vehicle entering the garage:

1. start fully off-screen to the right;
2. travel right-to-left toward the fixed parked X position;
3. rotate both wheels forwards according to linear distance travelled and effective wheel radius;
4. decelerate naturally into the parked position;
5. stop wheel rotation when linear movement reaches zero.

For a vehicle leaving the garage:

1. start at the fixed parked position;
2. move left-to-right in reverse toward the open garage door;
3. rotate both wheels backwards according to reverse distance travelled;
4. continue until the whole vehicle is off-screen right;
5. remove that vehicle from the visible garage composition.

Wheel angle is derived from distance travelled rather than an arbitrary looping timer so the tyres cannot visibly slide across the floor.

Conceptually:

`wheel_angle += distance_travelled / wheel_radius`

with signed distance preserving forward/reverse rotation direction.

Input that would mutate garage occupancy should be locked or deliberately queued while a drive-in/drive-out transition is in progress so the scene cannot end in an impossible mixed state.

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
      bumper_front.png
      bumper_rear.png
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
- independent front/rear bumper visibility from persistent vehicle state;
- per-model asset lookup by car ID;
- explicit garage occupancy state separate from ownership/selection state;
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
- baking either bumper permanently into the body layer;
- showing a static composed screenshot instead of independent scene and vehicle layers;
- pre-rendering the drive-in or drive-out animation into images/video;
- automatically replaying the arrival animation every time the garage screen opens;
- instantly replacing one physically parked car with another without the outgoing reverse / incoming forward sequence;
- showing two parked cars in the garage during a swap;
- generating a fresh raster source asset every time the player selects a paint colour;
- animating wheels independently of actual distance travelled.

## Current design decision

The neutral-grey-master approach is the required design. Backyard Racer has one permanent realistic garage scene and an explicit physical garage-occupancy state. If the garage is empty, a car drives in from the right. If another car is already parked and the player buys or moves a different car into the garage, the current car first reverses out to the right and only then does the replacement drive in from the right. A car already parked remains parked until a real occupancy change occurs.

## Removable bumpers

Every owned car tracks front and rear bumper fitment independently. Both ends are fitted when a car is acquired. Removing or refitting either end is a garage operation with an explicit cash cost. The fitment state is saved with the owned car and survives reloads.

Bumper work changes the rendered vehicle immediately. It does not replace the car artwork with a second full-car bitmap: the corresponding bumper layer is simply omitted or restored while the same body, wheels, paint and fixed details remain in place.
