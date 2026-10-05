# TODO

Written 2026-10-02 from a read-through of `engine/` and `very_cool_rpg/` at commit `16fc01a`.
Nothing here was built or run; every item comes from reading the code. Line numbers are from that commit.

Order matters: section 1 is small bugs, section 2 and 3 are the structural gaps that block gameplay,
section 4 is new subsystems that depend on them.

## 1. Fix first

- [ ] **Clamp the frame delta.** `engine/src/core/Application.cpp:166-185`
  `accumulator += deltaTime` has no cap. After a breakpoint, a window drag or a slow load, one frame
  carries seconds of time and the loop runs hundreds of fixed updates in a burst. Clamp `deltaTime`
  to something like 0.25s before adding it.

- [ ] **Render interpolation.** `engine/src/core/Application.cpp:179-196`
  Physics steps at 60 Hz, rendering is capped at 165 fps, and the camera is set in `OnFixedUpdate`
  (`GameplayLayer.cpp:98-103`), so movement judders on a high-refresh monitor. Standard fix: keep
  previous and current position, render at `alpha = accumulator / timeStep`. Decide early, it
  changes what `TransformComponent` holds and what `OnRender` receives.

- [ ] **Null level crash.** `very_cool_rpg/src/layers/GameplayLayer.cpp:95`
  If the world fails to load, `OnAttach` returns early at line 47 and `OnFixedUpdate` dereferences
  `*m_currentLevel` anyway. `OnRender` checks for null, this path doesn't. Line 115 has the same
  shape with `m_player` (no validity check before `GetComponent`).

- [ ] **Load failures terminate the game.**
  In SFML 3 `sf::Texture(path)` throws on failure, so the `if (m_texture) ... else LOG_E` in
  `engine/src/graphics/TileSet.cpp:11-17` and `:25-31` is dead code. `ldtkProject.loadFromFile`
  (`very_cool_rpg/src/loaders/world_loader.cpp:29`) also throws. Nothing catches either.
  `EntityDefinitionLoader.cpp:54` doesn't check the file exists the way `TileLayerBuilder.cpp:27` does.

- [ ] **Flipped tiles are lost in Y-sorted layers.** `engine/src/renderer/render2d.cpp:73-78`
  `TileInstance` has `flipX`/`flipY`, `QuadProps` doesn't, so a flipped tile in a `_YS_` layer draws
  unflipped. The current level has no flipped tiles, so it hasn't shown up.

- [ ] **Stuck keys with the debug UI (unconfirmed).** `engine/src/debug/DebugLayer.cpp:27-33`
  If imgui grabs the keyboard while a key is held, the `KeyReleased` is consumed and `InputLayer`
  never sees it, so the key stays down. Release events should probably never be consumed.
  Try: hold W, click into an imgui text field, release W.

## 2. Entity lifetime and level ownership

The biggest structural gap. Blocks projectiles, pickups, deaths and level transitions.

- [ ] **Entity destruction.**
  `Registry` has `CreateEntity` and no destroy, and nothing calls `SpatialGrid::EraseEntity`. Two
  places already work around it with `reg.valid()` checks: `engine/src/ecs/systems/physics_system.cpp:180`
  and `engine/src/debug/modules/CollisionOverlay.cpp:261`. `EntityFactory` is "the single place an
  entity comes into existence"; there needs to be a matching single place where one leaves, which
  also removes it from the grid. Remove the two `valid()` workarounds once it exists.

- [ ] **Entities need to belong to a level.**
  The loader spawns every level's entities into one registry (`world_loader.cpp:42-44`), but
  `UpdateKinematics` iterates all of them against the current level's grid
  (`physics_system.cpp:199-213`). Rendering filters by `layerUid`, which is the LDtk layer
  *definition* uid (`EntityLayerBuilder.cpp:19,32`) and is shared across levels. Invisible with one
  level; with two, level B's entities draw and simulate inside level A.
  Options to weigh: a level component on every entity, a registry per level, or spawning a level's
  entities only when it becomes active.

- [ ] `World::GetLevel(uint32_t)` vs `LevelId::uid` being `int32_t` (`World.h:12`, `Level.h:15`). Pick one.

## 3. Collision results

- [ ] **Contacts out of the physics step.** `physics_system.cpp:90-116`
  `MoveAndResolve` finds what was hit and throws it away. `ColliderRef` already carries the entity
  and tile index. Foundation for doors, pickups, damage and level transitions.

- [ ] **Tile flags are never set.** `very_cool_rpg/src/loaders/world/collision_baker.cpp:50`
  `TileShape::flags` and `AddBox(box, flags)` exist but the baker always passes the default 0.
  Needed for triggers and one-way or non-solid tiles.

- [ ] **Collision filtering** (layers/masks, solid vs trigger). Nothing exists yet.

- [ ] **Depenetration.** `engine/src/math/physics.cpp:59-69`
  An entity that starts overlapping gets `time = 0` and loses velocity on that axis every step, so
  it's stuck. The F2 dragon spawn at +50,+50 (`GameplayLayer.cpp:114-120`) can land inside a rock.

## 4. Missing subsystems, in order

- [ ] **Sprite animation.** Self-contained; everything after it looks broken without it.
- [ ] **One asset/texture cache.** `EntityDefinitionLoader.cpp:17-28` and `TileSetManager` cache
  separately, so a PNG used by both a tile layer and an entity is loaded twice.
- [ ] **Level transitions.** Needs sections 2 and 3.
- [ ] **Text/UI rendering.**
- [ ] **Audio.**

## 5. Hygiene

- [ ] **Project reference from the game to the engine.** The lib currently arrives by a pre-build
  copy (`very_cool_rpg.vcxproj:134-136`) with no build-order guarantee, so it can go stale.
- [ ] **Delete or fix the Win32 configs.** They have no include paths or C++20 set.
- [ ] **Test project.** `CellRange`, `SweptAABB`, `SpatialGrid` and `CollisionLayerBuilder` are pure
  and already had bugs that only review caught (see `docs/reports/physics_system_review*.md`).
- [ ] **Decide what `Texture2D` is for.** `render2d.cpp:60,108` casts it straight back to
  `SFMLTexture2D`, and the public API exposes `sf::` types everywhere, so it isn't hiding SFML.
  Either drop the interface or commit to it.
- [ ] **Physics lives in three places:** `sc::ecs::physics_system`, `sc::physics` (only `ColliderRef`)
  and `sc::math` (`SweptAABB`, `SpatialGrid`, which depends on entt). Consolidate when touching it next.

## Later, only when it shows up

- [ ] **Render hot path.** `engine/src/renderer/render2d.cpp:82-118`
  One draw call per quad, a `shared_ptr` copy per tile per frame, no culling, and an unstable sort
  that can flicker quads sharing a Y. Fine at 23 Y-sorted tiles.
- [ ] **Entity layer draw is O(layers x sprites).** `engine/src/ecs/systems/render_system.cpp:11-26`
- [ ] **Logging.** Several allocations and writes per line, no flush (a crash loses the tail), and
  `GameplayLayer.cpp:124` logs every frame when the level is null.
