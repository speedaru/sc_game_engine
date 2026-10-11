# TODO

Rewritten 2026-10-11 from the code review at commit `be1548a`. A number in brackets is a section of
`docs/reports/code_review_2026-10-11.md`, which has the reasoning and the snippets. Line numbers are
from that commit. Finished items from the previous list were removed.

Sections are in the order to work on them. Sections 1 to 4 are cleanup with no new behaviour,
5 to 8 are features, 9 to 11 can be done at any point.

Decisions this list assumes:

- **World.** Large rectangular levels that scroll, camera on the player. Separate areas (caves,
  secret rooms) are separate levels reached by a teleport. Instant cut for now, a fade later.
- **Loading.** Every level stays loaded and only the current one is simulated. That is what makes
  levels persistent, so no streaming and no per-level registries.
- **Scale.** A few hundred entities per level at most, about ten levels per world.
- **Interaction.** Walk into it, and stand next to it and press a key. Any entity may trigger things.
- **Combat.** Melee and projectiles. Enemies can have several hitboxes with different effects.
- **Animation.** A fixed layout on the sprite sheet.

## 1. Bugs

- [x] **Movers without a collider never move.** `engine/src/ecs/systems/physics_system.cpp:285-289` (2.1)
  The view requires `BoxColliderComponent`, which the factory only adds when the definition has
  hitboxes. View `Transform + Velocity` and make the collider optional in the loop.

- [x] **Entity types without a tileset cannot be spawned.** `very_cool_rpg/src/loaders/world/EntityDefinitionLoader.cpp:52-55` (2.2)
  The `continue` runs before `SetDefinition`. Blocks every invisible entity (trigger zones, camera
  zones, spawners). Once fixed, `EntityRegisterCollisions` (`physics_system.cpp:254-257`) must stop
  warning about a missing collider (2.9).

- [x] **A tile hitbox marked as trigger is baked solid.** `very_cool_rpg/src/loaders/world/collision_baker.cpp:49-51` (2.3)
  Skip trigger hitboxes in the baker and log once. Whether tiles get triggers at all is in section 9.

- [x] **`m_showMovementVec` is read uninitialised.** `very_cool_rpg/src/debug/PlayerMovement.h:28` (2.4)

- [x] **The loader reads one world by a hardcoded name.** `very_cool_rpg/src/loaders/world_loader.cpp:48` (2.14)
  `getWorld("World")` throws if the world is renamed, and the call is outside the `try`. Loop over
  `ldtkProject.allWorlds()` inside it.

- [x] **A failed texture load is silent.** `engine/include/engine/graphics/SFMLTexture2D.h:14-33` (2.13)
  The result is a valid 0 by 0 texture that gets cached. Return `nullptr` from `CreateTexture2D`
  and test for it in `TileSet.cpp:10-11`, `:18-19`.

- [x] **`m_player` can dangle.** `engine/include/engine/ecs/Entity.h:18`, `very_cool_rpg/src/layers/GameplayLayer.cpp:132,163` (2.5)
  `operator bool` only checks for null and is not `explicit`. Guard with `IsEntityValid`.

- [x] **Camera bounds and zoom.** `engine/src/graphics/Camera2D.cpp:14-16,34,37` (2.11)
  Centring on a small level ignores `min`. `SetZoom` multiplies the current zoom instead of setting it.

- [x] **`SC_ENABLE_DEBUG_TOOLS` changes the layout of `Application`.** `engine/include/engine/core/Application.h:47-51` (2.12)
  Make the member unconditional.

- [ ] **Flipped tiles are not drawn flipped.** `very_cool_rpg/src/loaders/world/TileLayerBuilder.cpp:40-42`
  `TileInstance` has no flip fields, and the builder logs a warning per flipped tile. Auto-rules
  produce flips as soon as a rule allows them, and every tile layer here is an auto-rule layer.

- [ ] **Small ones** (2.16)
  - `engine/src/world/World.cpp:5-7`: `AddLevel` takes a null level and replaces a duplicate uid silently.
  - `very_cool_rpg/src/loaders/world/HitboxUtils.cpp:23-24`: a hitbox with no size silently becomes 16 by 16.
  - `HitboxUtils.cpp:57`: the column count ignores tileset spacing and padding.
  - `very_cool_rpg/src/loaders/world/collision_baker.cpp:78-83`: a pure `AutoLayer` is drawn but not baked.
  - `engine/src/ecs/systems/physics_system.cpp:190`: the back-off before a hit is a fraction of time,
    so the gap scales with speed. Use a distance, with the same `SKIN` as depenetration.

## 2. Header hygiene

- [x] **`EntityHandle.h`.** New file `engine/include/engine/ecs/EntityHandle.h` (9.1 A)
  `Level.h` reaches all of entt and all of SFML Graphics through `Collisions.h` including
  `Registry.h` for one alias. `Collisions.h`, `SpatialGrid.h`, `lifecycle_system.h` and the game's
  `collision_system.h` include the new header instead.

- [x] **Make `Components.h` light.** `engine/include/engine/ecs/Components.h:6,39-40` (9.1 B)
  Delete the unused second `SpriteComponent` constructor, forward-declare `Texture2D`, include
  `<SFML/Graphics/Rect.hpp>` only.

- [x] **Narrow the entt and SFML includes.** (9.1 C, 9.2)
  `Registry.h:9` needs `<entt/entity/registry.hpp>`. `Window.h`, `SFMLTexture2D.h` and `TileLayer.h`
  include the SFML umbrella header.

- [x] **No global names from public headers.** (9.3)
  `namespace fs = std::filesystem;` in four engine headers and four game headers, and
  `using logging::LogLevel;` in `Application.h:9`. `ASSETS_DIR` in the game's `constants.h:18`
  should be `inline const`.

- [x] **Headers must compile on their own.** (9.5)
  About twenty headers use `std::` names or fixed-width integers they don't include. The engine ones
  matter most: they are compiled with the game's PCH, not the engine's.

- [ ] **Forward declarations instead of includes.** (9.4)
  Tables for both projects in the report. Do it after `EntityHandle.h`.

- [x] **Remove unused includes.** (9.6)

- [x] **Precompiled headers.** `engine/src/pch.h:27`, `very_cool_rpg/src/pch.h:26` (4.9, 9.7)
  The engine PCH includes nlohmann json and never uses it. The game PCH includes LDtk, so every
  game file can see LDtk types. The game PCH lacks `<algorithm>` and `<cmath>`.

## 3. Dead code

- [x] **Delete the leftovers.** (10)
  Engine: `physics_system::Teleport` (duplicates `MoveToLevel`, 4.5), the `SpatialGrid::Query`
  overload that returns a vector, `Grid::TryGetCell` and `GridCell`, `LayerRenderType`,
  `LayerType::Collision`, `class TileMap;` in `render2d.h:22`, `debug::Register<T>`, the
  `SPD_ENABLE_TRACKED_ALLOC` block in `logging.cpp:9-13`, `Window::m_data`.
  Game: `EntityDefinition::type`, `EntityRefValue::entityType`, `LoadContext::registry`,
  `FeatureTest::DrawMovementeVec`, the empty `HandleBlockContact`.

- [ ] **Decide on the unused API.** (10)
  `ViewCustom`, the from-memory texture and tileset constructors, `Clear` on three classes, the
  unused logging macros and `ADD_CLASS_TAG`'s `SetTag`, mouse bindings. Keep what has a planned user.

## 4. Game structure

In this order, each one makes the next smaller.

- [ ] **Request queue out of `ContactState`.** `very_cool_rpg/src/systems/collision_system.h:17-20` (5.3)
  `GameplayLayer` owns the queue and passes it to every system that may queue. `ContactState`
  keeps only the cooldown table. The type becomes `GameRequest` and leaves `layers/`.

- [ ] **`Spawn(type, level, params)`.** `very_cool_rpg/src/factories/EntityFactory.h:36` (5.4)
  Replaces the ambient `SetCurrentLevel`, set from four places. The factory finds the entity layer
  itself, so `m_entityLayerUid` and `FindEntityLayerUid` go.

- [ ] **One `EnterLevel`, and the start level from the player.** `very_cool_rpg/src/layers/GameplayLayer.cpp:73-96,180-183` (5.5, 2.6)
  The level-change sequence is written twice. The start level is the hardcoded `"World_Level_0"`
  while the player is found in any level. Take the level from the player's `LevelComponent`.
  Longer term: a `PlayerStart` marker, and the player spawned by code.

- [ ] **Blueprints as functions, and optional.** `very_cool_rpg/src/blueprints/` (5.1)
  Eight files for four classes of one to three lines. `using BuildFn = void(*)(Entity&, const SpawnParams&)`,
  all in one `blueprints.cpp`. An entity with no blueprint spawns from its definition alone, so a
  rock or a tree needs no C++.

- [ ] **Slim `GameplayLayer`.** (6.1)
  Input bindings to a free function. The F2 and F3 test hooks to buttons in the `FeatureTest`
  module, which removes two input actions.

- [ ] **One `LevelLocation` struct.** (8.4)
  `TeleportComponent`, `TeleportRequest` and `EntityRefValue` are the same two fields.

- [ ] **Store the LDtk `iid` of placed entities.** `very_cool_rpg/src/loaders/world/EntityLayerBuilder.cpp:94-103` (11.6)
  Into `SpawnParams` and a small component. Saving needs an identity that survives a restart, and
  it is cheap now and expensive after inventory and quests exist.

- [ ] **Input action overloads that take the enum.** `engine/include/engine/input/Input.h`, `InputContext.h` (5.7)
  Removes twelve casts, two of which cast to the wrong type.

- [ ] **Tear down on detach.** `very_cool_rpg/src/layers/GameplayLayer.cpp:55,188-189` (2.7)
  No `OnDetach`: the input context, the debug context and two debug modules outlive the layer.
  Needs a `debug::UnregisterModule`. Not urgent until a second layer (menu) exists.

## 5. Interaction

- [ ] **Interact input that reaches the fixed step.** (2.8)
  "Just pressed" is per rendered frame, so reading it in `OnFixedUpdate` misses most presses.
  Read the edge in `OnUpdate`, set a flag on a component, consume it in the fixed step.

- [ ] **Trigger areas drawn in LDtk.** `very_cool_rpg/src/entities/SpawnParams.h:13` (5.6)
  Hitboxes exist per entity type only. An instance with no hitboxes from its definition gets one
  trigger box of its own size (`ldtk::Entity::getSize()`). Needs the tileset-less fix from section 1.

- [ ] **Handler table for overlap contacts.** `very_cool_rpg/src/systems/collision_system.cpp:33-59` (5.2)
  An array of handler functions, each testing for its own component, with the cooldown keyed per
  handler. "Who may trigger this" becomes data on the trigger instead of a `PlayerTag` test in
  each handler. Write handlers as effects that don't care how they were triggered: the direction
  is a "when" component (overlap, interact, once, every N seconds) and "what" components
  (teleport, pick up, camera).

- [ ] **Enter and exit for overlaps.** `very_cool_rpg/src/systems/collision_system.cpp`
  A handler only knows "overlapping this step". Needed for a prompt while in range and for "once
  on entering". Needs last step's pairs to compare against.

- [ ] **`Entity::TryGetComponent` and `Registry::RemoveComponent`.** (6.5)

- [ ] **Enum and point fields from LDtk.** `very_cool_rpg/src/loaders/world/EntityLayerBuilder.cpp:53-74` (11.7)
  `ReadFields` skips `Enum`, `Point`, `Color`, `FilePath` and arrays. A chest needs an enum to
  say which item it holds.

## 6. Big levels

- [ ] **Load time.** A 15000 by 10000 level takes 5 to 6 seconds to load. Not measured yet.
  Check a Release build first, then time the LDtk parse, the tile layer build and the collision
  bake separately. The per-tile "flipped tiles not supported" warning is a suspect.

- [ ] **View culling.** `engine/src/renderer/render2d.cpp:82-118`, `engine/src/ecs/systems/render_system.cpp:11-26`
  Every Y-sorted tile and every sprite of the level is submitted and drawn, on screen or not.
  Test against the view rectangle before `SubmitQuad`.

- [ ] **Destroy temporary entities on level exit.**
  A level freezes when the player leaves, so a projectile in flight is still there on return.
  A marker component, and `SwitchLevel` destroys what carries it.

- [ ] **Level edges.** (11.9)
  Nothing stops an entity leaving the level rectangle, and outside it there are no collisions.
  Border tiles do the job today. A clamp in the physics step is the guarantee.

- [ ] **Y-sort of objects taller than one cell.** (11.2)
  A `_YS_` layer sorts each tile on its own, so a two-tile tree splits around the player. Anything
  taller than a cell that the player walks behind should be an LDtk entity. Needs optional
  blueprints from section 4.

- [ ] **Camera.** `engine/src/graphics/Camera2D.cpp` (11.8)
  No zoom level, smoothing, shake or zones, no window resize handling, and no pixel snapping
  (shimmering seams between tiles). A trigger that "does something to the camera" needs a camera
  driven by state, not assigned every frame.

- [ ] **A moved entity keeps its old `layerUid`.** `engine/src/ecs/systems/lifecycle_system.cpp:28-50` (2.10, 4.6)
  The sprite's layer is an LDtk layer definition uid. Breaks when the destination level has that
  layer hidden. Decide when a level first has two entity layers.

- [ ] **Chunked tile layers and batched Y-sorted quads.** Only if measured after culling.

## 7. Animation

- [ ] **Sprite animation.** (11.5)
  A component with frames and a timer, and a system that rewrites `SpriteComponent::rect`. Rows
  are states or directions, columns are frames, and the frame count comes from the blueprint.

- [ ] **Facing direction.** `CharacterController::direction` is this step's input and returns to
  zero when the keys are released. Animation needs the last non-zero one.

## 8. Collision redesign, then combat

Design the first three together, they change the same two structs (11.4).

- [ ] **Several hitboxes on one entity.** `engine/include/engine/physics/Collisions.h`, `engine/include/engine/math/Hitbox.h`
  An id or tag on `math::Hitbox`, set in the editor, and the index of both boxes on `Contact`.
  The cooldown key is then decided per handler: a coin made of two boxes counts once, a boss
  whose boxes are different attacks handles each on its own.

- [ ] **Collision filtering with layers and masks.**
  Blocking is all or nothing today, so a projectile can't pass through its owner.

- [ ] **Detect without blocking or moving.**
  Only a mover's solid box detects, and only another's trigger box. A sword swing needs a box
  that detects without colliding with walls, and a standing enemy needs to detect a player who
  is not moving.

- [ ] **Move physics into one module.** (4.1, 4.2)
  It is spread over `sc::ecs::physics_system`, `sc::physics`, `sc::math` (`SpatialGrid`, `Hitbox`,
  `physics.h`) and `sc::world`. Do it with this redesign, not on its own.

- [ ] **Melee and projectiles.**

## 9. Decisions still open

- [ ] **`Texture2D` as an interface.** `engine/src/renderer/render2d.cpp:60,108` (4.4)
  Recommendation: make it a concrete class that owns an `sf::Texture`. That also removes
  `SFMLTexture2D.h` from the public headers (4.3).

- [ ] **One texture cache.** `engine/src/graphics/TileSetManager.cpp:6` (2.15)
  `TileSet`'s tile size, rows and columns are never read, and the cache is keyed by path only,
  so it is a texture cache already. `EntityDefinitionLoader` keeps a second one.

- [ ] **Trigger and flagged tiles.** `engine/src/world/CollisionLayerBuilder.cpp`
  Anything with behaviour is an entity today. If that stays the rule, remove `TileShape::flags`
  and the `flags` parameter of `AddBox`, which no caller sets.

- [ ] **Layer tag format.** `very_cool_rpg/src/loaders/world/LayerTags.cpp:11-17` (2.16)
  Every `_`-terminated word becomes a tag, so `_YS_Trees_Big` has the tags `YS` and `Trees`.
  `TileLayerBuilder.h:15` documents a format the parser does not accept.

- [ ] **Engine constants.** `engine/include/engine/constants.h` (4.8)
  `LOG_FILE_NAME` and `MAX_FPS` are application settings. The `CREATE_CONST` macro hides a
  plain `constexpr`.

## 10. File moves and naming

Each move is also a `.vcxproj` and filters edit.

- [ ] **Game tree.** (7.2)
  `entities/` takes `EntityFactory` and the blueprints, `loaders/` becomes `ldtk/`, `enums/` is
  split into `input/` and `utils/`, `LayerRequest.h` leaves `layers/`.

- [ ] **Engine tree.** (7.1)
  `ecs/systems/` goes: `render_system` to `renderer/`, `lifecycle_system` to `world/` or `ecs/`,
  `physics_system` with the physics move in section 8. `Grid.hpp` to `Grid.h`, `ILevelLayer.h` to
  `LevelLayer.h`.

- [ ] **File names.** (8.1)
  Files of free functions in `PascalCase`: `HitboxUtils`, `LevelLoader`, `EntityDefinitionLoader`,
  `LayerTags`, `BlueprintRegistry`.

- [ ] **Namespaces.** (8.2)
  The folder to namespace mapping in the game follows no single rule. `logging` is global, not
  `sc::logging`.

- [ ] **Names.** (8.3, 8.4, 8.5)
  Noun-first physics functions, `IsActionActive` against `IsActionDown`, the `Component` suffix,
  `TagComponent` against `*Tag`, `ColliderRef`, `PhysicsScratch`, `g_` against `s_` statics,
  spelling.

- [ ] **Style.** (8.6)
  Spaces instead of tabs in eleven files, C-style casts, `using namespace` at file scope,
  `const` on by-value parameters, long comments under `debug/`. A one-off build at `/W4`.

## 11. Project hygiene

- [ ] **Project reference from the game to the engine.** The lib arrives by a pre-build copy
  (`very_cool_rpg.vcxproj:135,158`) with no build-order guarantee, so it can go stale.
- [ ] **Delete or fix the Win32 configs.**
- [ ] **Test project.** `CellRange`, `SweptAABB`, `Penetration`, `CellsFromBox`, `SpatialGrid` and
  `CollisionLayerBuilder` are pure.
- [ ] **Tracked files that should not be.** `very_cool_rpg/log.txt`, `last_session.txt`,
  `lib/sc_game_engine.pdb`, the LDtk `*_crash` backups (7.3).
- [ ] **`docs/reviews/` and `docs/reports/`** hold the same kind of document. Merge them.
- [ ] **Hitbox JSON schema.** Read by the game and written by the editor with no shared code.
  Describe it in `docs/` (4.10).
- [ ] **README.** How to build and run, including the working directory.

## Later, only when it shows up

- [ ] **Text and UI rendering.** Dialogue is the first hard requirement. Also needs a way for a
  layer to pause the layers below it (11.7).
- [ ] **Inventory and NPCs.** Game-side, no engine change.
- [ ] **Saving.** Uses the `iid` from section 4.
- [ ] **Screen transitions.** A fade on level switch, as a second request type.
- [ ] **Audio.**
- [ ] **Game states.** Pause, menu, restart.
- [ ] **A hook when an entity is destroyed.** Drops, quest counters, saved state.
- [ ] **`ViewLevel` scans every entity of every level.** `engine/include/engine/ecs/Registry.h:108-119`
  Fine at the planned scale.
- [ ] **Entity layer draw is O(layers x sprites).** `engine/src/ecs/systems/render_system.cpp:11-26`
- [ ] **Logging.** Several allocations and writes per line, and no flush, so a crash loses the tail.
- [ ] **Allocation in input and the grid.** Three `unordered_set`s per device, and a
  `std::function` on the grid's move path (6.6).
