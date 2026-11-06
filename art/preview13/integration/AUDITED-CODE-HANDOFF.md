# Integrating the prepared art into the audited code

1. Preserve the audited code and its fixes. Compare its building/resource
   registry to `day-resource-bindings.json`; map renamed types explicitly.
   Preserve each native animation slot count and the supplied image pivots.
2. Merge day assets and reference-only Help/selector changes. The full data
   snapshot in this pack is a reference: do not overwrite newer edited Help,
   GUI or simulation code wholesale. The precise earlier replacements are
   retained under `art01/`.
3. Apply the Church display-wording patch. Retain compatible resource/save
   identifiers and all model pixels. Public names/descriptions say Church.
4. Integrate the pedestrian direction/frame PNGs or the measured procedural
   fallback scale, never both. Use the supplied smoke-tip mapping for redesigned
   coal/light factories. New skyscraper economics and unlock conditions need
   explicit building definitions; artwork alone does not add jobs or housing.
5. Copy the small `runtime/data/night` folder into release app data and load
   `night/lighting.json`. `layers[*].atlas` and `rect` locate packed crops. Do
   not load raw master images or authoring crops in the game.

## Exact lighting placement

Each lit object contains the same image slot order as its day resource. A
frame has a day texture path, the native pivot, and up to two light references:

```text
day_top_left = screen_ground_anchor - day_pivot * zoom
light_top_left = day_top_left + offset_from_sprite_top_left * zoom
light_size = atlas_rect_size * zoom
```

Use the same camera, floor position, parent animation frame, camera shake and
zoom for both layers. Never recenter the cropped overlay by its own dimensions.
Do not stretch it to the size of the building or tile.

Darken the world sprite using the selected palette, then draw that sprite's
light layers immediately at the same depth position. Foreground sprites drawn
after it must still occlude its windows. Hide both parent and light together.
Do not tint HUD/menu art or dim the light layers a second time.

If the current implementation draws a dark fullscreen rectangle after all
world sprites, adjust that pipeline before adding lights. Drawing every light
over the finished scene without matching occlusion would make lights show
through nearer buildings and trees.

## Time and operating state

- `night_amount`: continuous dusk-to-night factor, zero by day.
- `deep_night`: zero for evening, approaching one late at night.
- `late`: persistent lamps/windows. Draw at `night_amount` opacity.
- `evening`: additional evening windows. Draw at
  `night_amount * (1 - deep_night)` opacity.
- These are disjoint emission groups, not duplicate full-night pictures.
- Apply power/occupancy/working-state factors to both groups. Electric fixtures
  require supply; early candle/lantern profiles may operate without electricity.
  The world-state adapter must use the audited simulation's real fields.
- Keep patterns stable for a given building; do not choose windows every frame.
- Existing Fire/FireWasteLand animations are explicitly self-emissive. Smoke,
  terrain and nature use world tint without invented lit windows.
- Native moving vehicle slots are mapped separately from the newer four-way
  traffic sprites. Draw their lights from the matching current vehicle frame.

Four optional reusable day lamp sprites have their own foot anchors and light
profiles. Place them only along actual appropriate infrastructure; do not add
traffic or lights to untouched wilderness. Their spacing/era and installation
cost, if any, are deliberate gameplay choices for the rewrite.

## Mayor work after the code arrives

Use `Mayor-Autonomy-Design.md` for Off/Advice/Autonomous control, district or
whole-map authority, budgets, need diagnosis, complete connected projects,
normal placement commands, cool-downs and result checking. None of that
controller is implemented by copying art files.

## Required graphical checks

Check a starter village, each civic/industrial family, Church, all six towers,
stations and native/new traffic at minimum/maximum zoom. Check dusk, night,
late night and dawn, loss of power, paused simulation, inactive buildings,
hide-buildings mode and overlap with foreground trees. Confirm animation
frames do not leave floating light crops behind. Confirm black ground gaps do
not return. Verify the custom menu background, theme, readable UI and speed
controls are preserved.
