# Code review: engine and game

Date: 2026-10-11. Commit: `be1548a`. Scope: `engine/` and `very_cool_rpg/`. The editor and `vendor/` are out.

Every header and source file in both projects was read in full (about 6,400 lines, 130 files), together with the earlier reports and `docs/TODO.md`. Nothing was built or run. A finding about an "unused" include or function was checked with a search over both projects, but not with the compiler, so treat those as "by reading".

A companion document, `docs/reports/ldtk_to_engine.md`, describes how the game reads an LDtk project into engine objects.

## How to read this

Each finding has a label and a size.

| Label | Meaning |
|---|---|
| **Real** | Wrong today, or wrong at the next obvious step (first entity of a kind, first second level, and so on). |
| **Latent** | Correct today only because of a circumstance that the planned game will remove. |
| **Taste** | Works and will keep working. A consistency or readability point. |

Sizes: **S** is minutes, **M** is about an hour, **L** is half a day or more. A size marked "+ vcxproj" means you also have to move or add files in Visual Studio.

Sections 2 to 10 follow the seven points you asked for. Section 11 looks at the project from outside, against the game you described. Section 12 is a suggested order of work. Section 13 has questions for you.

---

## 1. The short version

The code is in better shape than its age suggests. The engine and game boundary holds (no LDtk in the engine), entity lifetime and level ownership are sound, and the physics step is correct for what it claims. Most findings are about three things:

1. **A handful of real bugs**, all small to fix. The one most likely to cost you an evening: an entity with a velocity but no hitboxes never moves (2.1).
2. **Header weight and hygiene.** One include chain makes almost every header in both projects pull in all of entt and all of SFML Graphics. One new 5-line header breaks that chain (9.1).
3. **Game-side structure that will resist the features you listed.** Adding an entity type takes two new files and four edits for what is often one line of behaviour (5.1). The request queue belongs to the collision system, so a second system cannot use it (5.3). Level state is kept in sync by hand in two places (5.5).

The ten findings I would act on first, in order:

| # | Finding | Label | Size | Section |
|---|---|---|---|---|
| 1 | Movers without a collider never move | Real | S | 2.1 |
| 2 | Entity types without a tileset cannot be spawned | Real | S | 2.2 |
| 3 | `PlayerMovement::m_showMovementVec` is read uninitialised | Real | S | 2.4 |
| 4 | A tile hitbox marked as trigger is baked as solid | Real | S | 2.3 |
| 5 | Start level and player are found independently of each other | Latent | S | 2.6 |
| 6 | `EntityHandle.h`, to stop `Collisions.h` dragging in the whole ECS | Real | S + vcxproj | 9.1 |
| 7 | Request queue out of `ContactState`, and out of `layers/` | Real | S | 5.3 |
| 8 | One `EnterLevel` function, and `Spawn` takes the level | Latent | M | 5.4, 5.5 |
| 9 | Blueprints as functions, and optional | Real | M + vcxproj | 5.1 |
| 10 | Store the LDtk `iid` on spawned entities, before saving is designed | Latent | S | 11.6 |

---

## 2. Bugs and correctness

Ordered by how soon each one bites.

### 2.1 Movers without a collider never move — Real, S

`engine/src/ecs/systems/physics_system.cpp:285-289` iterates entities that have `TransformComponent`, `VelocityComponent` **and** `BoxColliderComponent`. `very_cool_rpg/src/factories/EntityFactory.cpp:64-67` only adds the collider when the definition has hitboxes.

So an entity type with no authored hitboxes and a `VelocityComponent` is skipped by the view and stays where it spawned. The branch written for exactly this case, `physics_system.cpp:162-165` ("if entity has no hitboxes, just move it"), can only be reached by an entity that has an empty collider component, which the factory never creates.

You will hit this with the first projectile, particle or NPC whose sprite tile has no hitbox yet, and it looks like an input or AI bug rather than a physics one.

Fix: make the collider optional in the loop instead of required by the view.

```cpp
auto movers = registry.ViewLevel<TransformComponent, VelocityComponent>(level.GetUid());

for (auto [entity, trans, vel] : movers) {
	trans.prevPos = trans.pos;

	const auto* col = registry.TryGetComponent<BoxColliderComponent>(entity);
	if (!col || !ColliderIsCollidable(*col)) {
		trans.pos += vel.velocity * timeStep;
		continue;
	}
	// existing Mover / MoveAndResolve / MoveEntity path
}
```

The early return in `MoveAndResolve` then becomes dead and can go.

### 2.2 Entity types without a tileset cannot be spawned — Real, S

`very_cool_rpg/src/loaders/world/EntityDefinitionLoader.cpp:52-55` hits `continue` before `factory.SetDefinition(...)` when the LDtk definition has no tileset. `EntityFactory::Spawn` then rejects the type with "no definition loaded" (`EntityFactory.cpp:42-46`).

This was item 4.1 in `entity_pipeline_refactor.md` and is still open. It is invisible today because the only tileset-less type, `WarpTarget`, is skipped as a marker (`EntityLayerBuilder.cpp:92`). It blocks every invisible entity you do want in the registry: trigger zones, camera zones, NPC talk areas, spawners.

Fix: store the definition first and skip only the texture and hitbox work.

Once fixed, 2.9 starts to matter.

### 2.3 A tile hitbox marked as trigger is baked as solid — Real, S

`very_cool_rpg/src/loaders/world/collision_baker.cpp:49-51` passes every hitbox of a tile to `CollisionLayerBuilder::AddBox`, which takes a rect and flags but no trigger (`engine/src/world/CollisionLayerBuilder.cpp:86-91` rebuilds the box from four floats). The `trigger` field is dropped and the box blocks.

The editor can already write `"trigger": true` (the chest tile in `3x3_entities.png` has one). That tileset is only used for entities, so nothing is wrong on screen today. The first trigger box on a tile that is used in a tile layer becomes an invisible wall.

Fix for now, one line in the baker: skip trigger hitboxes and log once. That matches the TODO entry "trigger and flagged tiles (decide, probably don't build)". If you decide to build them, `AddBox` needs the flag.

### 2.4 `m_showMovementVec` is read uninitialised — Real, S

`very_cool_rpg/src/debug/PlayerMovement.h:28` declares `bool m_showMovementVec;` with no initialiser. It is read at `PlayerMovement.cpp:27` (the checkbox) and `:35`. Reading an indeterminate `bool` is undefined behaviour. In an MSVC debug build it happens to be `0xCC`, so the arrow shows as soon as the module is enabled. Give it `= false`.

### 2.5 `m_player` can dangle — Latent, S

`sc::ecs::Entity::operator bool` (`engine/include/engine/ecs/Entity.h:18`) only tests the handle against `entt::null`. It does not know whether the entity still exists. `GameplayLayer` keeps the player as an `Entity` member and guards with `if (!m_player)` at `GameplayLayer.cpp:132` and `:163`, then calls `GetComponent` at `:137`, `:165` and `:184`.

Nothing destroys the player today (F3 only matches the `"Dragon"` name). The first death, or a save reload that respawns the player, leaves a non-null handle to a dead entity, and `GetComponent` on it is undefined behaviour in release and an entt assert in debug.

Two changes:

- Make `operator bool` `explicit`. As written, an `Entity` converts silently to `int` and can be compared with another `Entity` through `bool`.
- Guard with `m_registry.IsEntityValid(m_player.GetHandle())`, or drop the member and find the player through a view where it is needed (section 6.1).

### 2.6 Start level and player are found independently — Latent, S

`GameplayLayer.cpp:73` picks the level by the hardcoded name `"World_Level_0"`. `GameplayLayer.cpp:88-92` picks the first entity with a `PlayerTag` in **any** level. Nothing ties the two together.

If the `Player` entity is placed in another level in LDtk, the camera follows an entity whose level is neither simulated nor drawn, and the game looks frozen. If two levels each contain a `Player`, the second becomes controllable whenever its level is current, because `player_input` iterates every `PlayerTag` in the level.

Fix: find the player first, then take the level from its `LevelComponent`. That also removes the hardcoded level name, which is one of the open points under "Level transitions" in the TODO.

Longer term, placing the player as a level entity is the fragile part. A `PlayerStart` marker that the game reads, with the player spawned by code, removes the "two players" case entirely.

### 2.7 Debug modules and the debug context outlive what they point to — Latent, M

`GameplayLayer.cpp:188-189` registers two modules that hold `*m_world`, `this` (in the teleport callback) and `m_physicsScratch` by reference. `debug::SetContext` stores raw pointers to the registry, level and camera (`engine/src/debug/debug_system.cpp:71`). Modules are only dropped in `debug::Shutdown`, which runs when the `DebugLayer` detaches (`debug_system.cpp:65-69`).

This is safe today for one reason: `Application::~Application` detaches top-down (`Application.cpp:53-55`), so the debug layer goes first, and the gameplay layer is never removed while the game runs. The first "back to main menu" that removes or replaces `GameplayLayer` leaves the modules and the context pointing at freed memory, and the next frame dereferences them.

`GameplayLayer` also pushes an input context in `OnAttach` (`GameplayLayer.cpp:55`) and has no `OnDetach` to pop it.

Fix when a second layer arrives: an `OnDetach` that pops the input context, clears the debug context and unregisters its own modules. That needs a `debug::UnregisterModule(name)`, which does not exist yet.

### 2.8 Edge-triggered input must not be read in the fixed update — Latent, design rule

`Application.cpp:171` clears the "just pressed" and "just released" sets once per rendered frame. The fixed update then runs zero or more times in that frame (`Application.cpp:182-187`). At 165 fps and 60 Hz, about two frames in three run no fixed step at all.

So `IsActionJustPressed` inside `OnFixedUpdate` misses most presses, and on a slow frame with two steps it fires twice. Your code is correct today: edges are read in `OnUpdate` (`GameplayLayer.cpp:136`, `:145`) and held state in the fixed step (`player_input.cpp:22-25`).

This is here because "press E to talk to an NPC" is the next place it will come up, and the natural spot to write it is next to `player_input` in the fixed step. The pattern that works: read the edge in `OnUpdate`, set a flag on a component (for example `wantsInteract` on the controller), and let the fixed step consume and clear it.

### 2.9 `EntityRegisterCollisions` warns for every entity without a collider — Real, S

`physics_system.cpp:254-257` logs a warning when the entity lacks a transform or collider. `EntityFactory.cpp:73` calls it for every spawned entity. Today that only affects types without hitboxes. After 2.2 is fixed, every invisible entity logs a warning at spawn. A missing collider is a normal case there, so return quietly.

### 2.10 A moved entity keeps its old `layerUid` — Latent, M

`SpriteComponent::layerUid` (`engine/include/engine/ecs/Components.h:31`) is the LDtk layer **definition** uid, and `render_system.cpp:17` only draws a sprite when it matches the layer being drawn. `lifecycle_system::MoveToLevel` (`engine/src/ecs/systems/lifecycle_system.cpp:28-50`) changes the level and position but not the `layerUid`.

That works because every level in one LDtk project shares the same layer definitions. It stops working in two cases you are heading towards:

- The destination level has that layer hidden. `LevelLoader.cpp:24` skips invisible layers, so no `EntityLayer` with that uid exists there and the player is not drawn.
- The destination comes from a second `.ldtk` file, where uids are unrelated.

The underlying issue is that a per-entity render property refers to an LDtk identifier. See 4.6 for the direction.

### 2.11 `Camera2D` bounds and zoom — Latent, S

- `engine/src/graphics/Camera2D.cpp:34` and `:37` centre the camera on `(max - min) / 2` when the level is smaller than the view. That ignores `min`. It should be `(min + max) / 2`. Correct today because `min` is always `{0, 0}`.
- `Camera2D::SetZoom` (`Camera2D.cpp:14-16`) calls `sf::View::zoom`, which multiplies the current size. Calling `SetZoom(2.f)` twice zooms by four. The name says "set". It has no callers yet, so fix it before the first one.

### 2.12 `SC_ENABLE_DEBUG_TOOLS` can change the layout of `Application` — Latent, S

`engine/include/engine/core/Application.h:47-51` adds a member under `#if SC_ENABLE_DEBUG_TOOLS`. `engine/include/engine/debug/debug_config.h:5` invites the user to "define it yourself before including anything from debug/ to override".

If the game defines it differently from how the engine library was built, the two disagree on `sizeof(Application)` and on member offsets. That is a silent ODR violation and shows up as memory corruption, not as a compile or link error. Make the member unconditional (one `bool`), or remove the invitation from the comment.

### 2.13 Load failures are no longer fatal, but they are quiet — Real, S

The exceptions are now caught (`SFMLTexture2D.h:14-19`, `:28-33`, `world_loader.cpp:29-35`). What remains:

- A texture that fails to load leaves a valid `Texture2D` of size 0 by 0. `CreateTexture2D` returns non-null, `TileSet.cpp:9-11` computes zero rows and columns, and `TileSetManager.cpp:13-14` caches it. The level renders nothing and the only trace is one log line.
- `world_loader.cpp:48` calls `ldtkProject.getWorld("")` outside the `try` block. See 2.14.

Fix: have `CreateTexture2D` return `nullptr` on failure so callers can react. The renderer and the factory already test for null (`render2d.cpp:58`, `:106`, `EntityFactory.cpp:59`). `TileSet.cpp:10-11` and `:18-19` dereference the result without a test and would need one.

### 2.14 The loader only reads the unnamed world — Latent, S

`very_cool_rpg/src/loaders/world_loader.cpp:48` asks for the world named `""`. That is the implicit world of a project that does not use LDtk's multi-worlds setting. Once you enable multi-worlds, every world has a name, the lookup fails, and the exception is not caught.

`ldtk::Project::allWorlds()` exists in the vendored loader. Iterate it. Details in the LDtk document.

### 2.15 The tileset cache ignores the tile size — Latent, S

`engine/src/graphics/TileSetManager.cpp:6` keys the cache by file path only. Your current project already uses one PNG as two LDtk tilesets with different grids: `Decorative_32` and `Decorative_64` both point at `12x12_decorative.png`. The second request gets the `TileSet` built with the first one's tile size.

It does no harm, because nothing reads the tile size, rows or columns (section 10). That is the real finding: `TileSet` is a texture with unused extras, and `TileSetManager` is a texture cache. This belongs with the TODO item "one asset/texture cache".

### 2.16 Smaller items

| Where | What | Label |
|---|---|---|
| `engine/src/utils/logging.cpp:50` | `va_end(args)` on the caller's list, which the caller ends again. Delete the line. | Real, S |
| `very_cool_rpg/src/loaders/world/HitboxUtils.cpp:57` | Column count ignores tileset spacing and padding. Open since `entity_pipeline_refactor.md` 4.5. All current tilesets have zero for both. | Latent, S |
| `HitboxUtils.cpp:23-24` | A hitbox with no `w` or `h` silently becomes 16 by 16. A missing size is an authoring error and should be logged. | Taste, S |
| `very_cool_rpg/src/loaders/world/collision_baker.cpp:78-83` | Only `IntGrid` and `Tiles` layers are baked. A pure `AutoLayer` is drawn (`TileLayerBuilder.cpp:59`) but its tile hitboxes are ignored. | Latent, S |
| `very_cool_rpg/src/loaders/world/LayerTags.cpp:11-17` | Every `_`-terminated word of a name that starts with `_` becomes a tag. `_YS_Trees_Big` has tags `YS` and `Trees`. `TileLayerBuilder.h:15` documents the format as `"Ground_YS_"`, which the parser does not accept. | Taste, S |
| `engine/src/ecs/systems/lifecycle_system.cpp:36` | `ent` (an `entt::entity`) goes through printf varargs without the cast used everywhere else. | Taste, S |
| `engine/src/world/World.cpp:5-7` | `AddLevel` dereferences without a null check and silently replaces a level with the same uid. | Taste, S |
| `engine/src/ecs/systems/physics_system.cpp:190` | The back-off before a hit is `0.001` of the step's **time**, so the gap left to the wall scales with speed and shrinks to nothing at low speed. The depenetration pass then cleans up. A back-off in distance, using the same `SKIN` constant, would make the two agree. | Taste, S |
| `engine/src/core/Application.cpp:20-30` | `#if _DEBUG == 1 ... #elif _DEBUG == 0` works in release only because an undefined macro evaluates to 0. `#ifdef _DEBUG ... #else` says what is meant. | Taste, S |
| `engine/include/engine/graphics/Texture2D.h:17`, `SFMLTexture2D.h:13` | The byte vector is taken by value twice, so loading from memory copies the file twice. Unused path today. | Taste, S |
| `very_cool_rpg/src/layers/GameplayLayer.cpp:157-160` | Logs every frame when the level is null. | Taste, S |

---

## 3. `docs/TODO.md` re-examined

Each entry checked against the code at `be1548a`. I did not edit the file.

### Section 1, "Fix first"

| Entry | TODO says | Now |
|---|---|---|
| Clamp the frame delta | done | **Done.** `Application.cpp:168`. |
| Render interpolation | done | **Done.** `prevPos`, `alpha` and the camera lerp at `GameplayLayer.cpp:163-167`. One cosmetic leftover: the debug overlays draw at `trans.pos` (`CollisionOverlay.cpp:162`), so hitboxes lead the sprite by up to one step. |
| Null level crash | done | **Done** for null. The dangling-handle variant is 2.5. |
| Load failures terminate the game | open | **Mostly done, can be ticked.** Both throwing paths are caught and `EntityDefinitionLoader.cpp:25` checks the file. The "dead code" remark no longer applies. What is left is 2.13 and 2.14. |
| Flipped tiles are lost in Y-sorted layers | done | **Not done. Reopen.** `TileInstance` has no flip fields (`engine/include/engine/world/TileLayer.h:11-14`) and `TileLayerBuilder.cpp:40-42` logs "flipped tiles not supported" once per flipped tile, for both layer types. Your levels have none today. LDtk auto-rules produce them as soon as a rule allows X or Y flip, and all four of your tile layers are auto-rule layers. |
| Stuck keys with the debug UI | done | **Done.** `DebugLayer.cpp:26-41` no longer consumes releases, and `InputLayer.cpp:23-25` clears on focus loss. |

### Section 2, "Entity lifetime and level ownership"

| Entry | Now |
|---|---|
| Entity destruction | **Done.** One gap for later: the game has no hook when an entity is destroyed. `m_player` (2.5) is the first victim. Drops, quest counters and saved state will be the next. |
| Entities belong to a level | **Done.** Cost to know about: `Registry::ViewLevel` (`engine/include/engine/ecs/Registry.h:108-119`) filters every entity of every level on each call, and four systems plus the renderer call it per frame. Fine at the current scale. It grows with the total number of loaded entities, not with the current level. |
| `GetLevel` integer type | **Done.** |

### Section 3, "Collision results"

| Entry | Now |
|---|---|
| Contacts out of the physics step | **Done.** |
| Solid vs trigger hitboxes | **Done** for entities. Tiles: see 2.3. |
| Repeat rate and dedup | **Done.** The text says the cooldown is "counted in fixed steps". It is in seconds: `collision_system.cpp:71` subtracts `timeStep`. |
| Depenetration | **Done.** The file reference in the entry points at `math/physics.cpp:59-69`. The pass is at `physics_system.cpp:110-150`. Known limit: deep embedding in a block of several solid cells. |
| Enter and exit for overlaps | **Open, and now next.** Interaction ("press E while inside the NPC's area") and combat ("hit once per swing") both need it. |
| Collision filtering with layers and masks | **Open.** |
| Several hitboxes on one entity | **Open, and now needed.** You described enemies with several hitboxes that do different things. That needs an id on `math::Hitbox` and the two box indices on `Contact`. Design it with filtering, as the entry says. |
| Trigger and flagged tiles | **Open.** New fact: 2.3. Also `TileShape::flags` and the `flags` parameter of `AddBox` are never set by any caller (section 10). |

### Section 4, "Missing subsystems"

| Entry | Now |
|---|---|
| Sprite animation | Open. |
| One asset/texture cache | Open. New evidence in 2.15. |
| Level transitions | Done. Open points: start level (2.6), only the player can use a well, and 2.10. |
| Text/UI rendering, Audio | Open. |

### Section 5, "Hygiene"

| Entry | Now |
|---|---|
| Project reference from the game to the engine | Open. The copy command is still at `very_cool_rpg.vcxproj:135` and `:158`. |
| Win32 configs | Open. Both project files still carry them. |
| Test project | Open. `Penetration` and `CellsFromBox` are two more pure functions that would be cheap to test. |
| Decide what `Texture2D` is for | Open. My recommendation is in 4.4. |
| Physics lives in three places | Open, and it is four now. See 4.1 and 4.2. |
| `Collisions.h` includes all of `Registry.h` | Open. The fix is 9.1. The comment about the tile index was fixed. |
| Request types live under `layers/` | Open. See 5.3. |

### "Later, only when it shows up"

All three entries still describe the code. One addition to "Render hot path": there is no culling at all, and a `TileLayer` is one vertex array for the whole level (`engine/src/world/TileLayer.cpp:6-51`). That is fine for a 768 by 640 level. It is the first thing a large island level will hit (11.1).

---

## 4. Code in the wrong module

### 4.1 `engine/ecs/systems/` groups by kind, not by subject — Real, M + vcxproj

The folder holds three files that belong to three different modules:

| File | What it is | Better home |
|---|---|---|
| `physics_system` | the physics step, plus grid registration | `engine/physics/` |
| `render_system` | draws a whole level, tiles and entities | `engine/renderer/` |
| `lifecycle_system` | destroys entities and moves them between levels, needs `World` | `engine/world/`, or leave in `ecs/` |

`render_system::RenderWorld` is not an ECS system. It walks a `Level`'s layers and only one branch touches the registry. Its namespace, `sc::ecs::render_system`, tells the reader to look in the wrong place.

### 4.2 Physics is spread over four namespaces — Real, L + vcxproj

| Piece | File | Namespace |
|---|---|---|
| the step | `ecs/systems/physics_system` | `sc::ecs::physics_system` |
| contact and collider types | `physics/Collisions.h` | `sc::physics` |
| sweep, penetration | `math/physics.h` | `sc::math` |
| hitbox | `math/Hitbox.h` | `sc::math` |
| spatial grid (depends on entt) | `math/SpatialGrid` | `sc::math` |
| static tile collision | `world/CollisionLayer`, `CollisionLayerBuilder` | `sc::world` |

`sc::math` should hold things with no idea what an entity is: `Lerp`, `Grid`, `CellRange`. `SpatialGrid` stores entity handles and `Hitbox` has a `trigger` flag, so both are physics.

Proposed: everything in the first five rows moves to `engine/physics/` and `sc::physics`. `CollisionLayer` can stay in `world/`, since a `Level` owns it. This is the biggest move in the report. It is worth doing once, when you next change physics for hitbox ids and filtering, not as a standalone cleanup.

### 4.3 `SFMLTexture2D.h` is in the public include folder — Real, S + vcxproj

It is an implementation detail of `Texture2D.cpp` and `render2d.cpp`. In the public folder it invites the game to include it and puts `<SFML/Graphics.hpp>` on offer to every game file. It belongs in `engine/src/graphics/`. This goes away if you follow 4.4.

### 4.4 `Texture2D` as an interface — recommendation: drop it

`render2d.cpp:60` and `:108` cast every `Texture2D` straight back to `SFMLTexture2D`. The public API uses `sf::Vector2f`, `sf::FloatRect`, `sf::IntRect`, `sf::Color`, `sf::RenderWindow` and `sf::Event` throughout. The interface does not hide SFML and there is no second backend.

What it costs today: a virtual base, a factory function, a public header that should be private, and a `shared_ptr` cast per quad per frame. What it would cost to commit to it instead: wrapping every SFML type in the public API, which is weeks of work for no feature.

Make `Texture2D` a concrete class that owns an `sf::Texture`. Keep the name. The decision that tips it: you are writing a game on SFML, not a renderer abstraction.

### 4.5 Two functions that both "place an entity" — Real, S

`physics_system::Teleport` (`physics_system.cpp:309-327`) and `lifecycle_system::MoveToLevel` (`lifecycle_system.cpp:28-50`) both set `pos` and `prevPos` and resync the spatial grid. `Teleport` has no callers. Either delete it, or have `MoveToLevel` call it for the same-level part. Keep one.

### 4.6 LDtk concepts that reached the engine — Taste to Latent

The engine has no LDtk types, which is the rule that matters. Three assumptions did cross over:

- `SpriteComponent::layerUid` is described in `render_system.cpp:16` as "the layer definition, shared across levels". That is an LDtk idea. The engine only needs "which entity layer of the level draws this sprite". Consequence in 2.10. A sprite could store a small sorting-layer index that each level maps to one of its layers, or sprites could draw at the level's only entity layer unless told otherwise. Decide when a level first has two entity layers.
- `GameplayLayer.cpp:34-43` (`FindEntityLayerUid`) is a query on `Level` written in the game. It wants to be `Level::FindLayer(LayerType)`.
- `engine/include/engine/debug/DebugDraw.h:19-20` describes what `GameplayLayer` does "today". The comment is stale and names a game class.

### 4.7 Game code that is generic — Taste

| Code | Verdict |
|---|---|
| `very_cool_rpg/src/enums/hash.hpp` (constexpr FNV-1a) | Generic. Move to `engine/utils/hash.h` when a second user appears. Until then it is at least in the wrong game folder (7.2). |
| `character_movement` (acceleration, friction, max speed) | Generic top-down movement. Leave it in the game. Pulling it into the engine before a second game exists would be guessing at its API. |
| `collision_system`'s cooldown table | Same. Leave it. |
| `FieldBag`, `FieldValue` | A property bag. Fine where it is. |

### 4.8 Policy that the engine hardcodes — Taste, S

`engine/include/engine/constants.h` holds three things with three different owners: `LOG_FILE_NAME` (an application setting), `MAX_FPS` (a window setting, and vsync is not offered at all), and `PREALLOCATED_QUAD_QUEUE` (a private detail of `render2d.cpp`). The first two belong in `WindowData` or an application config struct. The third belongs in `render2d.cpp`.

The `CREATE_CONST` macro (`constants.h:4`) hides a plain `constexpr` declaration behind a name that leaks into every file. `constexpr float PHYSICS_HZ = 60.f;` is shorter and greppable.

### 4.9 The precompiled headers blur the boundaries — Real, S

- `engine/src/pch.h:27` includes `nlohmann/json.hpp`. The engine does not use JSON anywhere.
- `very_cool_rpg/src/pch.h:26` includes `LDtkLoader/Project.hpp`. Every game file can therefore see LDtk types, so the rule "only the loaders see LDtk" (`EntityDefinitionLoader.h:14-16`) is kept by discipline and not by the build. `GameplayLayer.cpp:4` includes it too and uses nothing from it. The loaders already include what they need, so removing it from the PCH costs nothing.

### 4.10 The hitbox JSON schema lives in two projects — note

`HitboxUtils.cpp:19-27` reads `{x, y, w, h, trigger}` and the editor writes it, with no shared code since the editor does not link the engine. The editor is out of scope here. The risk is drift: a field added on one side and not the other. A ten-line description of the schema in `docs/` would be the cheap guard.

---

## 5. Modularity: where a new feature means editing a central place

### 5.1 Adding an entity type — Real, M + vcxproj

Today a new LDtk entity needs:

1. an enumerator in `entities/EntityType.h`
2. `blueprints/XxxBlueprint.h`
3. `blueprints/XxxBlueprint.cpp`
4. a line in `BlueprintRegistry.cpp`
5. two entries in the `.vcxproj` and its filters

The four existing blueprints have bodies of one to three lines and no state. And an LDtk entity with no blueprint is dropped with a warning per instance (`EntityFactory.cpp:35-39`), so a rock that only needs a sprite and a hitbox still needs all five steps.

Three changes, in order of value:

**Make the blueprint optional.** If no blueprint is registered, spawn from the definition alone. A decoration, a rock or a sign then needs zero C++. This is also the answer to your question about decorations of different sizes (11.2).

**Make a blueprint a function.** The interface has one method and no implementation has members.

```cpp
using BuildFn = void(*)(sc::ecs::Entity& entity, const entities::SpawnParams& params);

factory.Register(EntityType::Chest, [](sc::ecs::Entity& entity, const entities::SpawnParams&) {
	entity.AddComponent<components::PickupTag>();
});
```

All of them then fit in one `blueprints.cpp`. Eight files become one, and a new type is one enumerator and one `Register` call.

**Later, build components from data.** `WellBlueprint` already does this in spirit: it reads a field and adds a component. The general form is "an entity with a `destination` field gets a `TeleportComponent`, whatever its type". Do not build this until two different entity types want the same component.

`EntityFactory.cpp:37` also still logs the raw hash when a blueprint is missing. The definition's `debugName` is right there.

### 5.2 `HandleOverlapContact` — Real, M, do it with the third behaviour

`very_cool_rpg/src/systems/collision_system.cpp:33-59`. With two behaviours the `if` chain is not the problem. These are:

- Each behaviour repeats "is `self` the player" (`:46`, `:52`).
- Each behaviour must live in this file.
- The cooldown check sits in the dispatcher (`:75-80`), so one behaviour's cooldown hides the pair from all the others.

The shape that removes all three, when you add the third behaviour:

```cpp
using OverlapHandler = void(*)(HandlerContext& ctx, ecs::EntityHandle self, ecs::EntityHandle other);

constexpr OverlapHandler OVERLAP_HANDLERS[] = { HandlePickup, HandleTeleport };
```

Each handler tests for its own component and returns early. Each can live in its own file. The dispatcher loops over the table. The cooldown key gains a handler index so the table is per behaviour.

This is the registry you proposed a few sessions ago, minus the map keyed by contact kind. I advised against building it then. With interaction, combat and NPCs on your list, the third behaviour is close and it is the right time.

The direction beyond that, for "a wide and flexible range of effects": split **when** from **what**. A trigger component says when something fires (on overlap, on interact, once, every N seconds, player only). Effect components say what happens (teleport, pick up, change camera). One system matches contacts to triggers, and one small function per effect runs it. Do not build it now. Keep it in mind while writing the handler table, so that handlers end up as "effect" functions that do not care how they were triggered.

### 5.3 The request queue belongs to the wrong owner — Real, S

`very_cool_rpg/src/systems/collision_system.h:17-20` puts two unrelated things in `ContactState`: the cooldown table, which is private state of the collision system, and `requests`, which is an output channel to the layer.

An interaction system or a dialogue system will want to queue a teleport or a camera change too. It cannot, without taking a `collision_system::ContactState`.

Fix:

- `GameplayLayer` owns `std::vector<GameRequest> m_requests` and passes it to each system that may queue.
- `ContactState` keeps only `rateLimits`.
- The type moves out of `layers/` and loses "Layer" from its name. A system including a header from `layers/` (`collision_system.h:7`) is a dependency pointing upwards.

The `std::variant` with `std::visit` is worth keeping. Adding a request type without an `Apply` overload fails to compile, which is the property you want.

### 5.4 `EntityFactory` has an ambient "current level" — Latent, M

`EntityFactory::SetCurrentLevel` (`very_cool_rpg/src/factories/EntityFactory.h:36`) is set from four places: `LevelLoader.cpp:15`, `world_loader.cpp:53`, `GameplayLayer.cpp:79` and `:182`. `Spawn` uses whatever was set last.

Consequences:

- You cannot spawn into a level other than the current one (a monster in the room the player is about to enter).
- A forgotten call spawns into the wrong level, or into none after `world_loader.cpp:53` resets it.
- The caller must also supply `layerUid` in `SpawnParams`, and the default of `-1` means "never drawn" with no warning (`SpawnParams.h:9-11`).

Fix: `Spawn(type, level, params)`. The factory can then resolve the entity layer from the level itself, and `GameplayLayer::m_entityLayerUid` and `FindEntityLayerUid` disappear.

### 5.5 Level state is synchronised by hand — Latent, S

Four things must change together when the current level changes: `m_currentLevel`, `m_entityLayerUid`, the factory's level, and the camera bounds. That sequence is written twice, at `GameplayLayer.cpp:73-85` and `:180-183`. A fifth piece of level state (music, ambient colour, a level-entered event) has to be added in both.

One private `EnterLevel(sc::world::Level& level)` called from both places. With 5.4 done it shrinks to two assignments.

### 5.6 Hitboxes exist per entity type only — Real for your plans, M

Hitboxes come from the custom data of the tile an entity type is drawn with (`EntityDefinitionLoader.cpp:61-72`). An instance cannot have its own size or boxes. `SpawnParams.h:13` has a TODO for this.

So you cannot draw a rectangle in LDtk and say "this area is a trigger". Secret-room entrances, camera zones and NPC talk areas all want that. LDtk supports it directly: an entity definition can be resizable, and `ldtk::Entity::getSize()` returns the instance size.

Suggested rule: if the instance has no hitboxes from its definition, give it one trigger box of its own size. That needs 2.2 first.

### 5.7 Input actions are cast at every call — Taste, S

There are ten `static_cast<int32_t>(InputAction::X)` in `GameplayLayer.cpp:49-54` and `player_input.cpp:22-25`, and two that cast to `uint32_t` instead (`GameplayLayer.cpp:136`, `:145`) although `ActionId` is `int32_t`. A template overload in `Input.h` and `InputContext.h` removes them all:

```cpp
template <typename Action> requires std::is_enum_v<Action>
bool IsActionActive(Action action) { return IsActionActive(static_cast<ActionId>(action)); }
```

### 5.8 Things that are fine as they are

- **The system order in `OnFixedUpdate`** (`GameplayLayer.cpp:106-121`). Explicit calls in one function are easier to reason about than a scheduler. Keep it.
- **`ReadFields`' switch** (`EntityLayerBuilder.cpp:53-74`). It grows with LDtk's list of field types, which is fixed, and not with your game. Missing today: `Point`, `Enum`, `Color`, `FilePath` and the array types. `FieldValue` has an `sf::Vector2f` alternative that nothing fills.
- **The `LayerType` switch in `render_system.cpp:41-59`**. Three cases, and layers are data that the renderer draws. An earlier review suggested a virtual `Draw`. I would not: it would make `world/` depend on the renderer. Add a `default` and drop the unused `LayerType::Collision`.
- **`LayerBuilderRegistry`**. More machinery than three builders need, but it is built and it works.
- **One `GameComponents.h`**. Split it by subject at around fifteen components, not before.

---

## 6. Responsibility: what does too much

### 6.1 `GameplayLayer` — Real, M

It currently does all of this:

| Job | Where | Verdict |
|---|---|---|
| builds the input bindings | `:48-55` | Move to a free function, `MakeGameplayBindings()`. |
| resolves asset paths, loads the world | `:57-67` | Stays. |
| picks the start level and finds the player | `:73-96` | 2.6. |
| runs the fixed-step schedule | `:99-122` | Stays. This is the layer's real job. |
| F2 spawn and F3 destroy test hooks | `:136-153` | Move into the `FeatureTest` debug module as buttons. That removes two input actions and the last use of `m_entityLayerUid`. |
| camera follow | `:163-167` | Stays until there is a second camera behaviour (zones, shake). Then it becomes a small system with a `CameraTarget` component, and `m_player` goes. |
| switches level | `:172-185` | 5.5. |
| applies requests | `:197-208` | Stays. 5.3 changes who owns the queue. |
| registers debug modules | `:187-190` | Stays. Needs the matching unregister (2.7). |

After 5.3, 5.4, 5.5 and the two moves above, the file is a loader, a schedule and a request handler. I would not introduce a `GameSession` or `WorldSession` class yet. There is one layer and one world, so the class would only rename the problem.

### 6.2 `collision_system` — covered by 5.2 and 5.3

It is a dispatcher, a cooldown table and every behaviour, in one file.

### 6.3 `physics_system.cpp` — Taste

At 328 lines it holds the step, the grid registration helpers (`EntityRegisterCollisions` and friends), the bounds helper and `Teleport`. The helpers are used by the factory, the lifecycle system and the debug overlay, so they are a small "collider and grid" API of their own. Splitting is not needed. If the file grows with hitbox ids and filtering, the helpers are the natural part to move out.

### 6.4 `Application` — Taste

It owns logger setup, renderer setup, imgui setup, the input layer and the debug layer. That is acceptable for an application class. `#if SC_ENABLE_DEBUG_TOOLS` appears six times in `Application.cpp`. A small private helper with four functions (init, event, begin frame, render) would bring it down to one place. Only worth it if you touch that file anyway.

### 6.5 `Registry` — Taste

`TakePendingDestroy` and `DestroyNow` are public and protected only by a comment (`engine/include/engine/ecs/Registry.h:34-38`). Fine at this size. The class also has no `RemoveComponent`, and `Entity` has no `TryGetComponent`. Both will be wanted soon (status effects, the "wants to interact" flag).

### 6.6 Code that is lower-level than it needs to be — Taste

| Where | What | Alternative |
|---|---|---|
| `very_cool_rpg/src/systems/character_movement.cpp:17-22`, `:30-33`, `:38-43` | hand-written length and normalise, three times | SFML 3 has `sf::Vector2f::length()` and `normalized()` |
| `engine/src/utils/logging.cpp:42-65` | `malloc`, `va_copy`, two `vsnprintf` calls per fragment, three or four fragments per line | one stack buffer per line, one write |
| `engine/src/input/InputState.cpp:38-40` | three `unordered_set`s per device, so a key press allocates | a fixed array or bitset indexed by key |
| `engine/include/engine/math/CellRange.h:30` | `std::function` parameter on the grid's move path | a template parameter |

None of these is a problem at the current scale.

---

## 7. File organisation

Everything is flagged, ordered by value. Each move costs a `.vcxproj` and filters edit, plus the include lines that mention the old path.

### 7.1 Engine

| Priority | Move | Why |
|---|---|---|
| High | `ecs/systems/physics_system.*` to `physics/` | 4.1 |
| High | `ecs/systems/render_system.*` to `renderer/` (and rename, for example `level_renderer`) | 4.1 |
| High | `math/SpatialGrid.*`, `math/Hitbox.h`, `math/physics.*` to `physics/` | 4.2. `math/physics.h` next to a `physics/` folder is the most confusing path in the tree. |
| Medium | `graphics/SFMLTexture2D.h` to `src/graphics/`, or remove it with 4.4 | 4.3 |
| Medium | `ecs/systems/lifecycle_system.*` to `world/` or `ecs/` | The `systems/` folder is then empty and can go. |
| Low | `math/Grid.hpp` to `Grid.h` | The only `.hpp` in the engine. |
| Low | `world/ILevelLayer.h` to `LevelLayer.h` | It is a base class with data and no virtual behaviour, not an interface (8.4). |
| Low | `math/math.h` | `<engine/math/math.h>` works, but the name is the C header's. `interp.h` or similar would be unambiguous. |

### 7.2 Game

Current tree, with the issues:

```
very_cool_rpg/src/
  blueprints/      8 files for 4 one-method classes, plus includes.h and a "registry" that is one function
  components/      fine
  debug/           fine
  entities/        the data types of the spawn pipeline
  enums/           one enum (InputActions.h) and one hash function (hash.hpp, not an enum)
  factories/       exactly one class
  layers/          GameplayLayer, and LayerRequest.h which is not about layers
  loaders/         world_loader.* alone at this level
  loaders/world/   everything else that touches LDtk
  systems/         fine
```

`blueprints/`, `entities/` and `factories/` are one subject, "how an entity is made", split by kind of class. `enums/` is a folder named after a language feature. `loaders/` has one occupant and one subfolder.

Proposed tree:

```
very_cool_rpg/src/
  main.cpp  pch.h  constants.h
  components/GameComponents.h
  debug/
  entities/        EntityType, EntityDefinition, SpawnParams, EntityField,
                   EntityFactory (from factories/), blueprints.h/.cpp (from blueprints/)
  gameplay/        GameRequest.h (from layers/LayerRequest.h)
  input/           InputAction.h (from enums/InputActions.h)
  layers/          GameplayLayer
  ldtk/            everything from loaders/ and loaders/world/
  systems/
  utils/           hash.h (from enums/hash.hpp)
```

A folder named `ldtk/` makes the boundary visible in the tree: if a file is not in it, it should not mention LDtk.

| Priority | Move | Why |
|---|---|---|
| High | `layers/LayerRequest.h` out of `layers/` | 5.3 |
| High | `blueprints/*` to one `entities/blueprints.*` | 5.1 |
| Medium | `factories/EntityFactory.*` to `entities/` | one subject, one folder |
| Medium | `loaders/` and `loaders/world/` to `ldtk/` | flattens a folder with one file and names the boundary |
| Medium | delete `blueprints/includes.h` | 9.4 |
| Low | `enums/hash.hpp` to `utils/hash.h`, `enums/InputActions.h` to `input/InputAction.h` | |
| Low | snake_case names for files of free functions | 8.1 |

### 7.3 Repository

| What | Why |
|---|---|
| `very_cool_rpg/log.txt`, `last_session.txt` | Tracked output files. |
| `very_cool_rpg/lib/sc_game_engine.pdb` | Tracked, while the matching `.lib` is ignored. It goes stale on every build. |
| `very_cool_rpg/assets/world_test1/backups/*_crash/` | LDtk crash backups, tracked. |
| `docs/reviews/` and `docs/reports/` | Two folders for the same kind of document. `CLAUDE.md` only names `docs/reports/`. |
| `docs/reports/debug_subsytem_plan.md` | Typo in the file name. |

---

## 8. Naming conventions

Checked against the conventions in `CLAUDE.md`. Listed by pattern. All of these are **Taste** unless noted. A consistent name is worth more than any particular choice, so each row says which form the rest of the code already uses.

### 8.1 Files

The rule is `PascalCase` for a class and `snake_case` for a namespace of free functions. These files hold only free functions and use `PascalCase`:

| File | Contains |
|---|---|
| `loaders/world/HitboxUtils.*` | `ParseHitboxes`, `ParseHitboxesFromCustomData`, `GetTileIdFromRect` (the last is not about hitboxes) |
| `loaders/world/LevelLoader.*` | `LoadLevel` |
| `loaders/world/EntityDefinitionLoader.*` | `LoadEntityDefinitions` |
| `loaders/world/LayerTags.*` | `ExtractLayerTags` |
| `blueprints/BlueprintRegistry.*` | `RegisterAll`. The registry itself is `EntityFactory`. |

`collision_baker`, `world_loader`, `player_input` and the others follow the rule.

Also: `InputActions.h` is plural, the enum is `InputAction`. Two files use `.hpp` (`Grid.hpp`, `hash.hpp`), all others `.h`. `includes.h` says nothing about its content.

### 8.2 Namespaces

In the game, the mapping from folder to namespace follows no single rule:

| Folder | Namespace |
|---|---|
| `components/`, `entities/`, `factories/`, `blueprints/`, `debug/` | `game::<folder>` |
| `systems/player_input.h` | `game::player_input` (the file name, with no `systems`) |
| `loaders/` and `loaders/world/` | `game::world_loader` |
| `layers/`, `enums/InputActions.h`, `constants.h` | `game` |
| `enums/hash.hpp` | `game::enums` |

In the engine:

- `logging` is a global namespace (`engine/include/engine/utils/logging.h:6`), not `sc::logging`. The `LOG_*` macros call `LogOutput` unqualified and only find it through argument-dependent lookup on `logging::LogLevel`.
- `sc::renderer::render2d` nests a free-function namespace inside a module namespace. `sc::ecs::physics_system` does the same. `sc::input` and `sc::debug` put their free functions directly in the module namespace. Pick one.
- System namespaces: the engine uses the `_system` suffix for all three. The game uses it for one of three (`collision_system`, but `player_input` and `character_movement`).

### 8.3 Functions

| Pattern | Examples | Rest of the code |
|---|---|---|
| noun first | `EntityGetBounds`, `EntityRegisterCollisions`, `EntityUnregisterCollisions`, `ColliderIsCollidable`, `EntityCollectCandidates` (all in `physics_system`) | verb first: `UpdateKinematics`, `FlushDestroyed`, `MoveToLevel`, `CollectCandidates` |
| two names for "held" | `input::IsActionActive` (`Input.h:15`), `InputContext::IsActionDown` (`InputContext.h:25`) | |
| two names for "this frame" | `IsKeyPressed` (`InputState.h:11`), `IsActionJustPressed` (`Input.h:18`) | "Pressed" alone reads as "held" to most people. |
| namespace repeated in the name | `player_input::UpdatePlayerInput`, `character_movement::UpdateCharacterMovement` | `lifecycle_system::FlushDestroyed` |
| name does not match the behaviour | `TileLayer::LoadTiles` (sets from a vector, loads nothing), `Camera2D::SetZoom` (multiplies), `LayerBuilderRegistry::Dispatch` (builds) | |
| lowercase function | `enums::hash` (`hash.hpp:7`), which also has a local variable named `hash` | `PascalCase` |

### 8.4 Types

| Pattern | Examples |
|---|---|
| component suffix | Engine: every component ends in `Component`. Game: `TeleportComponent`, but `CharacterController`, `PlayerTag`, `PickupTag`. |
| "tag" means two things | Engine `TagComponent` holds a name string (`Components.h:12-14`) and `GameplayLayer.cpp:147` matches on it. Game `*Tag` types are empty markers. `NameComponent` would end the overlap. |
| `I` prefix on a non-interface | `ILevelLayer` has data members, a protected constructor and no virtual methods apart from the destructor. |
| names that describe something else | `ColliderRef` is a resolved box held by value, not a reference. `PhysicsScratch` also holds `contacts`, which is the step's output and is read by the game and by a debug module. `LayerRequest` is not about layers. `FeatureTest` is a level-teleport panel. |
| `struct` versus `class` | `EntityLayerBuilder` is a `struct`. The other two builders are classes. |
| three structs with the same two fields | `components::TeleportComponent` (`GameComponents.h:10-14`), `TeleportRequest` (`LayerRequest.h:10-13`) and `entities::EntityRefValue` (`EntityField.h:12-16`) are all "a position in a level". One `LevelLocation { int32_t levelUid; sf::Vector2f pos; }` used by all three removes two field-by-field copies. |
| field naming | `LevelComponent::uid`, but `levelUid` everywhere else. |

### 8.5 Variables and constants

| Pattern | Examples | Rest of the code |
|---|---|---|
| file-level statics | `g_logLevels`, `logFile` (`logging.cpp:6-7`), `g_IndentLevel`, `g_pauseLog` (`logging.h:37-38`) | `s_` prefix: `s_contexts`, `s_modules`, `s_quadQueue`, `s_keys` |
| two different `EPSILON`s | `physics_system.cpp:11` (a fraction of a time step), `CollisionLayerBuilder.cpp:9` (pixels) | name them for what they measure |
| snake_case locals | `filter_fn` (`Registry.h:112`, `:125`), `args_copy` (`logging.cpp:44`) | `camelCase` |
| lowercase type alias | `using entity = entt::entity;` (`SpatialGrid.h:17`) | `EntityHandle` |
| spelling | `TILE_VERTICE_COUNT`, `DrawMovementeVec`, "bouds" (`TileSet.cpp:24`) | |
| reserved identifier | `__RELATIVE_FILE__` (`logging.h:4`). Names with a double underscore belong to the compiler. |

### 8.6 Style

| Pattern | Where |
|---|---|
| spaces instead of tabs | `render_system.h`, `InputContext.h`, `ILevelLayer.h`, `TileLayer.h`, `YSortedTileLayer.h`, `render2d.cpp`, `logging.cpp`, `TileLayer.cpp`, `EntityField.h`, parts of `GameplayLayer.cpp`, one line of the game's `constants.h` |
| quoted includes | `#include "includes.h"` in the four blueprint headers |
| `static` instead of an anonymous namespace | `render2d.cpp:10-15`, `logging.cpp:6-7` |
| trailing return types | only `SpatialGrid` (`SpatialGrid.h:33`, `:52`) |
| C-style casts | `Components.h:40`, `TileSet.cpp:25`, `:31`, `logging.cpp:54`, `:56` |
| `sf::Vector2f` by value in some signatures and by const reference in others | by value: `Teleport`, `MoveToLevel`, `CellsFromBox`. By reference: `SweptAABB`, `Camera2D::SetPosition`, `SetBounds`. It is two floats. By value everywhere is the simple rule. |
| `const` on by-value parameters and returns | `Camera2D(const sf::Vector2f size)`, `const sf::Vector2f GetSize() const` (`Camera2D.h:13`, `:22-23`), `GetLevel(const int32_t uid)` |
| `#pragma region` | only `GameplayLayer` |
| compiler-specific keyword | `__forceinline` on a member defined in a `.cpp` (`InputContext.cpp:33`). It does nothing useful there. |
| `using namespace` at file scope | `using namespace sc;` (`collision_system.cpp:9`), `using namespace game;` (`main.cpp:14`), `using namespace sc::debug;` inside `game::debug` in three headers |
| comment style | The files under `debug/` carry multi-paragraph comments. The convention is short, lowercase, and about why. |
| member initialiser order | `ILevelLayer.h:34-35` initialises `m_type` before `m_id`, declared the other way round. `logging.h:70` does the same with `m_pPauseLog` and `m_prevVal`. Harmless here, and a warning at `/W4`. |

Both projects build at `/W3`. A one-off build at `/W4` would list the unused parameters (`params` in every blueprint, `argc`, `ctx`) and the initialiser orders above for free.

---

## 9. Includes

### 9.1 The chain that makes everything heavy — Real, S + vcxproj

```
world/Level.h
  includes world/CollisionLayer.h
    includes physics/Collisions.h
      includes ecs/Registry.h        (only for the EntityHandle alias)
        includes <entt/entt.hpp>     (all of entt)
        includes ecs/Components.h
          includes <SFML/Graphics.hpp>   (all of SFML Graphics)
          includes graphics/Texture2D.h  (and with it <filesystem>)
```

So `Level.h`, `World.h`, `CollisionLayer.h`, `physics_system.h`, and every game header that includes any of them, compile all of entt and all of SFML Graphics. The precompiled header hides the cost in build time. It does not hide the coupling: a change to `Components.h` rebuilds the world loader.

Three small changes break the chain.

**A. New header `engine/ecs/EntityHandle.h`:**

```cpp
#pragma once
#include <entt/entity/entity.hpp>

namespace sc::ecs {
	using EntityHandle = entt::entity;
}
```

`SpatialGrid.h:9` already includes that entt header on its own, so it is known to work. Then `Collisions.h`, `SpatialGrid.h`, `lifecycle_system.h` and the game's `collision_system.h` include `EntityHandle.h` instead of `Registry.h`, and `Registry.h` includes it too. This is the single change with the most effect. It also closes the TODO entry about `Collisions.h`, and lets `SpatialGrid` and `physics_system.cpp:29` use `EntityHandle` instead of `entt::entity`, which is what the comment at `Registry.h:14` asks for.

**B. Make `Components.h` light.** The second `SpriteComponent` constructor (`Components.h:39-40`) is the only reason the header needs the full `Texture2D` type. It has no callers. Delete it, forward-declare `Texture2D`, and replace `<SFML/Graphics.hpp>` with `<SFML/Graphics/Rect.hpp>`. `Components.h` then costs a few small headers, and `Registry.h` can go on including it for `LevelComponent`.

**C. Narrow the entt includes.** `Registry.h:9` needs `<entt/entity/registry.hpp>`, not `<entt/entt.hpp>`. `Entity.h:4` needs nothing from entt beyond what `Registry.h` gives it.

### 9.2 The SFML umbrella header in public headers — Real, S

| Header | Has | Needs |
|---|---|---|
| `core/Window.h:5` | `<SFML/Graphics.hpp>` | `<SFML/Graphics/RenderWindow.hpp>` |
| `ecs/Components.h:6` | `<SFML/Graphics.hpp>` | `<SFML/Graphics/Rect.hpp>` |
| `graphics/SFMLTexture2D.h:8` | `<SFML/Graphics.hpp>` | `<SFML/Graphics/Texture.hpp>` |
| `world/TileLayer.h:5` | `<SFML/Graphics.hpp>` | `<SFML/Graphics/VertexArray.hpp>`, `<SFML/Graphics/Rect.hpp>` |

### 9.3 Global names declared in public headers — Real, S

A header that declares a name at global scope gives it to every file that includes it, directly or not.

| Where | What |
|---|---|
| `graphics/Texture2D.h:6`, `SFMLTexture2D.h:3`, `TileSet.h:9`, `TileSetManager.h:9` | `namespace fs = std::filesystem;` |
| `core/Application.h:9` | `using logging::LogLevel;` |
| game: `constants.h:6`, `LoadContext.h:11`, `world_loader.h:11`, `EntityDefinitionLoader.h:6` | `namespace fs = std::filesystem;` |
| both `pch.h` | `namespace fs`, `namespace chrono` |

In engine public headers, write `std::filesystem::path` in full. The game may keep one alias, in one place. The PCH is the wrong place for it, because a header that relies on it is no longer self-contained.

The game's `constants.h:18` also defines `const fs::path ASSETS_DIR` in a header. Each file that includes it gets its own copy, constructed at start-up. Make it `inline const`.

### 9.4 Headers that include a whole header for one name

Forward declarations are enough wherever a type is only used by reference, pointer or smart pointer.

**Engine**

| Header | Includes | Replace with |
|---|---|---|
| `core/ILayer.h` | `RenderWindow.hpp`, `Event.hpp` | `namespace sf { class RenderWindow; class Event; }` |
| `ecs/systems/physics_system.h` | `Registry.h`, `SpatialGrid.h`, `Collisions.h`, `Level.h`, `math/physics.h` | `EntityHandle.h`, `<SFML/Graphics/Rect.hpp>`, and forward declarations of `Registry`, `Entity`, `SpatialGrid`, `Level`, `PhysicsScratch`. `math/physics.h` is not used in the header at all. Line 11 forward-declares `Registry` after including it. |
| `ecs/systems/render_system.h` | `RenderWindow.hpp`, `Registry.h`, `Level.h`, `Camera2D.h` | four forward declarations |
| `ecs/systems/lifecycle_system.h` | `Registry.h` | `EntityHandle.h`, `<SFML/System/Vector2.hpp>`, forward declaration of `Registry` |
| `renderer/render2d.h` | `TileLayer.h`, `YSortedTileLayer.h` | forward declarations. Also remove `class TileMap;` (`:22`), which names a class that does not exist. |
| `world/TileLayer.h` | `graphics/TileSet.h` | forward declaration (held by `shared_ptr`) |
| `math/CellRange.h` | `Grid.hpp` | only `GridSize` is needed. Acceptable as it is. |

**Game**

| Header | Includes | Replace with |
|---|---|---|
| `blueprints/includes.h` | `Entity.h`, `Components.h`, `GameComponents.h`, `IEntityBlueprint.h`, `SpawnParams.h`, `EntityField.h` | Delete the file. Each blueprint header needs only `IEntityBlueprint.h`. The `.cpp` files include what they use. Gone entirely with 5.1. |
| `entities/EntityDefinition.h`, `loaders/world/TileColliderCache.h`, `loaders/world/HitboxUtils.h` | `engine/ecs/Components.h` | `engine/math/Hitbox.h`. All three only need `Hitbox`. |
| `loaders/world/HitboxUtils.h` | `nlohmann/json.hpp` | Nothing. `ParseHitboxes(const json&)` is only called inside `HitboxUtils.cpp`. Make it file-local and the header no longer mentions JSON. |
| `loaders/world/LoadContext.h` | `Registry.h`, `TileSetManager.h`, `Level.h`, `EntityFactory.h` | Forward declarations. All members are references. `Level.h` is not used at all, and neither is the `registry` member. |
| `loaders/world/ILayerBuilder.h` | `LoadContext.h`, `LDtkLoader/Layer.hpp` | forward declarations |
| `loaders/world/LevelLoader.h` | `LDtkLoader/Level.hpp`, `engine/world/Level.h`, `LoadContext.h` | forward declarations |
| `loaders/world/EntityDefinitionLoader.h` | `LDtkLoader/Project.hpp` | `namespace ldtk { class Project; }` |
| `loaders/world/collision_baker.h` | two LDtk headers, `CollisionLayerBuilder.h` | Forward declarations. `BakeTileLayer` and `BakeIntGridLayer` are only called inside the `.cpp`, so they can leave the header and take `CollisionLayerBuilder.h` with them. |
| `loaders/world_loader.h` | `World.h`, `Registry.h`, `TileSetManager.h`, `EntityFactory.h` | four forward declarations |
| `factories/EntityFactory.h` | `engine/world/Level.h` | forward declaration. `<string>` and `<string_view>` are unused. |
| `blueprints/BlueprintRegistry.h` | `EntityFactory.h` | forward declaration |
| `debug/ContactOverlay.h` | `engine/physics/Collisions.h` | forward declaration of `PhysicsScratch` (reference member) |
| `layers/GameplayLayer.h` | `loaders/world_loader.h` | Move to the `.cpp`. |
| `systems/collision_system.h` | `engine/ecs/Registry.h` | `EntityHandle.h` and a forward declaration |

### 9.5 Headers that only compile because of the PCH or another include

A header should compile when it is the first include of an empty file. These do not.

| Header | Uses without including |
|---|---|
| `engine/constants.h` | `size_t` (`<cstddef>`) |
| `ecs/Components.h` | `std::shared_ptr` (`<memory>`), `int32_t` (`<cstdint>`) |
| `ecs/Entity.h` | `std::forward` (`<utility>`) |
| `ecs/Registry.h` | `std::is_empty_v` (`<type_traits>`) |
| `ecs/systems/lifecycle_system.h` | `sf::Vector2f`, `int32_t` |
| `graphics/Camera2D.h` | `std::optional` (`<optional>`) |
| `graphics/Texture2D.h`, `TileSet.h` | `uint32_t`, `uint8_t` |
| `graphics/SFMLTexture2D.h` | `std::vector`, `std::exception` |
| `math/SpatialGrid.h` | `std::optional` |
| `physics/Collisions.h` | `assert` (`<cassert>`), `sf::Vector2f` |
| `utils/logging.h:2` | `va_list`. It includes `<memory>` "for va_list". The header is `<cstdarg>`. |
| `world/Level.h` | `std::move` (`<utility>`), `uint32_t` |
| `world/TileLayer.h`, `YSortedTileLayer.h` | `std::vector` |
| `world/World.h` | `std::shared_ptr`, `std::string`, `int32_t` |
| game: `components/GameComponents.h`, `constants.h`, `systems/*.h` | `int32_t`, `uint32_t` |
| game: `debug/FeatureTest.h` | `std::function`, `std::string_view`, `std::vector` |

Source files have the same issue with algorithms. `collision_system.cpp:72`, `:77` use `std::remove_if` and `std::find_if`, and `character_movement.cpp` uses `std::sqrt` and `std::max`. The game's `pch.h` includes neither `<algorithm>` nor `<cmath>`, so these compile through entt's own includes.

The engine case matters more than the game case. Engine public headers are compiled inside the game with the **game's** PCH, so an engine header that leans on the engine's PCH works by coincidence.

### 9.6 Includes that are not used

| File | Unused |
|---|---|
| `engine/input/Input.h` | `<vector>` |
| `engine/math/CellRange.h` | `<vector>` |
| `engine/math/Grid.hpp` | `<SFML/System/Vector2.hpp>` |
| `engine/math/SpatialGrid.h` | `<functional>` |
| `engine/renderer/render2d.h` | `<vector>` |
| `engine/graphics/Texture2D.h` | `<string>` |
| `engine/ecs/Entity.h` | `engine/utils/logging.h` |
| `engine/src/core/Application.cpp` | `engine/input/Input.h` |
| `engine/src/ecs/systems/lifecycle_system.cpp` | `engine/ecs/Entity.h` |
| `engine/src/world/Level.cpp`, `TileLayer.cpp` | `engine/utils/logging.h` |
| `engine/src/debug/DebugLayer.cpp` | `engine/graphics/Camera2D.h` |
| `very_cool_rpg/src/layers/GameplayLayer.cpp` | `LDtkLoader/Project.hpp`, `engine/graphics/Texture2D.h`, `engine/renderer/render2d.h`, and the aliases `core` (`:26`) and `gfx` (`:30`) |
| `very_cool_rpg/src/debug/FeatureTest.cpp` | `DebugDraw.h`, `Registry.h`, `Entity.h`, `lifecycle_system.h`, `Camera2D.h`, `GameComponents.h`, the commented-out include at `:11`, and the `ecs` alias |
| `very_cool_rpg/src/debug/FeatureTest.h`, `constants.h` | `<SFML/Graphics/Color.hpp>`, `<SFML/System/Vector2.hpp>` |
| `very_cool_rpg/src/loaders/world/LayerTags.cpp` | `engine/utils/logging.h` |
| `ChestBlueprint.cpp`, `WellBlueprint.cpp` | the `ecs` alias |

### 9.7 The precompiled headers

| | Finding |
|---|---|
| Engine | `nlohmann/json.hpp` is never used. The `chrono` alias is never used. `<format>` is used twice, both in `CollisionOverlay.cpp`. |
| Game | `LDtkLoader/Project.hpp` and `nlohmann/json.hpp` are only needed by the loaders (4.9). `<algorithm>` and `<cmath>` are missing although used. |

A PCH should hold standard and vendor headers that most files use and that rarely change. It should not be what makes your own headers compile.

---

## 10. Unused code

Each row was checked with a search over both projects. "Unused" means no caller today. Rows marked *API* are reasonable to keep if you expect to use them. The others are leftovers.

### Engine

| What | Where | Note |
|---|---|---|
| `physics_system::Teleport` | `physics_system.cpp:309` | 4.5 |
| `Camera2D::SetZoom` | `Camera2D.cpp:14` | *API*, and wrong (2.11) |
| `TileSet::GetTileRect`, `GetGridWidth`, `GetGridHeight`, `GetTileSize`, `m_rows`, `m_cols` | `TileSet.h:17-28` | 2.15 |
| `TileSet(const std::vector<uint8_t>&, ...)`, `CreateTexture2D(std::vector<uint8_t>)` | `TileSet.h:15`, `Texture2D.h:17` | *API* |
| `TileSetManager::Clear`, `SpatialGrid::Clear`, `Grid::Clear` | | *API* |
| `SpatialGrid::Query` returning a vector | `SpatialGrid.h:33` | the buffer overload replaced it |
| `Grid::TryGetCell`, `GridCell` | `Grid.hpp:9-37` | |
| `Registry::ViewCustom` and `ViewCustomImpl` | `Registry.h:84-91`, `:121-133` | *API* |
| `LayerRenderType`, `LayerType::Collision` | `ILevelLayer.h:10`, `:13-16` | |
| `class TileMap;` | `render2d.h:22` | no such class |
| `TileShape::flags`, the `flags` parameter of `AddBox`, `CollisionLayerBuilder::m_flags` | | never set by a caller. See the TODO entry on flagged tiles. |
| `debug::Register<T>` | `debug_system.h:25-28` | all call sites use `RegisterModule` |
| `SetTag` from `ADD_CLASS_TAG`, and the tag itself | `logging.h:94-97` | Two classes have a tag. Neither changes it. `Registry.cpp:8` prints it twice in one line. |
| `LOG_T`, `LOG_OBJ_T`, `LOG_OBJ_D`, `LOG_SCOPE`, `LOG_PAUSE*`, `ScopedLog`, `ScopedPauseLog` | `logging.h` | |
| the `SPD_ENABLE_TRACKED_ALLOC` block | `logging.cpp:9-13` | From another project. The macros it uses do not exist here. |
| `Window::m_data`, `~Window() {}` | `Window.h:17`, `:23` | `Window` adds nothing over `sf::RenderWindow` today. |
| `input::PopContext`, `ClearContexts`, `IsActionJustReleased`, mouse bindings, `GetMousePosition` | | *API* |

### Game

| What | Where | Note |
|---|---|---|
| `EntityDefinition::type` | `EntityDefinition.h:19` | written at `EntityDefinitionLoader.cpp:44`, never read |
| `EntityRefValue::entityType` | `EntityField.h:15` | written, never read |
| `LoadContext::registry` | `LoadContext.h:16` | never read |
| `FeatureTest::DrawMovementeVec` | `FeatureTest.h:29` | declared, never defined |
| the `sf::Vector2f` alternative of `FieldValue` | `EntityField.h:18` | nothing produces it |
| `InputAction::SpawnEnt`, `DestroyEnt` | | test hooks (6.1) |
| `HandleBlockContact` | `collision_system.cpp:30-31` | empty body, called for every block contact |

---

## 11. From outside: what the game you described needs

You described a Zelda-style 2.5D game: a large island, secret rooms reached by teleport into another LDtk world, interaction with objects that can have many kinds of effect, simple combat with multi-hitbox enemies, NPCs, an inventory, animation, saving, and UI and audio later. Against that, in the order I think the gaps will be felt:

### 11.1 The shape of the island is the biggest open decision

The engine simulates and draws exactly one level at a time, in that level's own coordinates. `sc::world::Level` has a size but no position in a world (`engine/include/engine/world/Level.h:24-33`). The loader drops the level's world position and its neighbours.

There are two ways to make a large island, and they lead to different engines:

| | One level per screen or region, with a transition at the edge | One seamless scrolling map |
|---|---|---|
| In LDtk | many levels laid out side by side (GridVania layout) | one very large level, or many levels drawn together |
| Engine today | fits as it is. A transition is a teleport to the neighbour. | does not fit |
| Needs | neighbour lookup (LDtk and the vendored loader both provide it), an edge trigger, and a scroll or fade | culling, chunked tile layers, and either one huge level or several levels simulated at once in shared coordinates |
| Cost | small | large |

The first is how the 2D Zelda games work, and it is nearly free here. If you want the second, the render path (one vertex array per layer for the whole level, no culling) and `ViewLevel` (a scan of all entities) are the first two things to change. I need your answer before recommending anything in the renderer (section 13).

### 11.2 Decorations of different sizes

You asked whether this is an LDtk problem. Partly.

**What already works.** Your project already has two decoration layers with different grids, `_YS_Decorations` at 32 and `_YS_Decorations2` at 64. The loader handles that correctly: tile positions arrive in pixels, so the layer's grid size does not matter to rendering, and the collision baker uses the smallest grid of the level (`collision_baker.cpp:17-36`). One layer per grid size is the standard LDtk answer, and it works here as it is.

**Where it breaks: Y-sorting of objects taller than one cell.** In a `_YS_` layer each tile is sorted on its own, by its own bottom edge (`engine/src/renderer/render2d.cpp:95-103`). A tree made of two stacked tiles is two independent quads. When the player stands behind the tree, level with the trunk, the whole tree should cover them. Instead the top tile's bottom edge is above the player's feet, so the canopy is drawn behind the player while the trunk is drawn in front. A tile layer cannot fix that, because it has no idea which tiles form one object.

**The answer for those objects: make them entities.** An LDtk entity can show any rectangle of a tileset, of any size, and is placed freely. It is sorted as one quad. With 5.1 (blueprint optional), a tree or a house is an LDtk entity definition and no C++. As far as I can tell from the vendored header, `ldtk::Entity::getTextureRect()` returns the tile of the instance, so one generic `Prop` definition with a per-instance tile may also be possible. I could not verify that from the header alone.

So: flat things and one-cell things in tile layers with a grid that suits them, anything taller than a cell that the player can walk behind as an entity.

A side note: the comment at `render2d.cpp:75`, "pivot is at the feet", is wrong. The pivot is `{0, 0}`, the top-left corner. The sort is still right, because it adds the height of the rect.

### 11.3 Interaction

"Interact with stuff, and the stuff can have very wide and flexible effects" needs four things that do not exist yet:

1. **An interact input that reaches the fixed step** (2.8).
2. **Enter and exit for overlaps** (the open TODO entry). "Show a prompt while in range" and "fire once on entering" both need last step's pairs.
3. **Trigger areas drawn in LDtk** (5.6 and 2.2).
4. **A way for an effect to reach beyond the registry** (5.3). The request queue is that way. Camera effects are the obvious second request type.

With those, the trigger and effect split in 5.2 has what it needs.

### 11.4 Combat with multi-hitbox enemies

- `math::Hitbox` needs an id or tag, set in the editor. `Contact` needs the index of the mover's box and of the other's box.
- The rule "only an entity with a velocity detects" (the open TODO entry on filtering) means a sword swing cannot hit a static pot, and a stationary enemy never detects a player standing in its attack box unless the player is the one moving. Combat needs hitboxes that detect without moving.
- Layers and masks, so that a projectile passes through its owner.
- "Different amounts of damage per hitbox" is game data keyed by the hitbox id. It fits the handler table of 5.2 with the per-handler cooldown.

Design the hitbox id, the contact indices and the filter together, as the TODO says. All three change the same two structs.

### 11.5 Animation

Self-contained in code: a component with frames and a timer, and a system that rewrites `SpriteComponent::rect`. The open question is where the frame data comes from. LDtk cannot author animations. The options are a small JSON file per sprite sheet, a convention on the sheet (row is direction, column is frame), or your own editor, which already edits per-tile data. A convention on the sheet is the cheapest and suits a character with four directions.

Animation also needs a facing direction, which nothing stores yet. `CharacterController::direction` is the input for this step and goes back to zero when the keys are released.

### 11.6 Saving: one cheap thing to do now

A save file has to say "this chest was opened" and "this pickup is gone". That needs an identity for a placed entity that is the same on every run. The runtime entity handle is not: it depends on spawn order.

LDtk gives every placed entity a permanent `iid`. `EntityLayerBuilder.cpp:94-103` does not pass it on. Adding it to `SpawnParams` and storing it in a small component costs a few lines today. Adding it after an inventory, quests and dialogue have all invented their own ways to refer to things costs a lot more. This is the one item in this section I would do before it is needed.

### 11.7 Inventory and NPCs

Both are game-side and need no engine change. They do need:

- **Enum fields from LDtk**, so a chest can say which item it holds. `ReadFields` skips them. The vendored loader exposes an enum value's name, so storing it as a string or its hash is the simple route, and you already have the hash function.
- **Text rendering and a UI layer** (an open TODO entry). Dialogue is the first hard requirement for it. The layer stack is ready for a UI layer above the gameplay layer.
- **A paused gameplay layer** while a dialogue or menu is open. Nothing supports "this layer blocks updates below it" yet. Events can be consumed, updates cannot.

### 11.8 Camera

There is no way to set a zoom level (2.11), no shake, no zones and no smoothing. The camera is assigned the player's position every frame (`GameplayLayer.cpp:163-167`). There is also no handling of a window resize, and the camera position is not snapped to whole pixels, which usually shows as shimmering seams between tiles once the camera moves at sub-pixel speeds. "Doing stuff to the camera" from a trigger needs a camera that is driven by state instead of assigned.

### 11.9 Level edges

Nothing stops an entity from walking out of a level. Outside the grid, `CellsFromBox` clamps to an empty range (`engine/src/math/CellRange.cpp:72-77`), so there are no tile collisions out there and the entity sits in zero cells of the spatial grid, where nothing can touch it. Today the tiles at the border do the job. With the first layout in 11.1, the edge is exactly where a transition trigger belongs, so this resolves itself there.

### 11.10 Things an outside reader would expect and not find

- **No tests** (an open TODO entry). The pure functions keep growing in number.
- **No `README` content on how to run it.** The working-directory requirement (`../../assets`) is only in `CLAUDE.md`.
- **No asset manager** (an open TODO entry).
- **No error path to the user.** Every failure is a log line. A missing asset leaves a black window.
- **No game state beyond "playing".** No pause, no menu, no way to restart.

None of these blocks the next feature. They are the usual gap between a working prototype and something another person can pick up.

---

## 12. Suggested order of work

Grouped so that each phase leaves the project building and nothing has to be done twice.

**Phase 0: bug fixes, no structural change. About an hour.**
2.1, 2.2 with 2.9, 2.3 (the one-line guard), 2.4, 2.11, the `va_end` line in 2.16. Correct the TODO entries listed in section 3.

**Phase 1: header hygiene, one new file. About two hours.**
9.1 A, B and C, then 9.2, 9.3 and 9.5. Do 9.1 first: after it, 9.4 gets much easier, because fewer headers need what they include. Then 9.4 and 9.6 as you touch each file.

**Phase 2: game structure, no engine change.**
In this order, because each one simplifies the next:

1. 5.3, the request queue.
2. 5.4, `Spawn` takes the level.
3. 5.5 and 2.6, `EnterLevel` and the start level taken from the player.
4. 5.1, blueprints as optional functions.
5. 6.1, the remaining `GameplayLayer` moves.
6. `LevelLocation` from 8.4.
7. 11.6, the `iid`.

**Phase 3: file moves. Do them in one sitting each, since every move touches the project files.**
The game tree of 7.2 first. It is cheaper and touches only one project. The engine moves of 7.1 and 4.2 are best done together with the next physics feature, not on their own.

**Phase 4: decisions, each needs an answer from you before any code.**
`Texture2D` (4.4), flipped tiles (section 3), trigger tiles (2.3), the layer tag format (2.16), and above all 11.1.

**Then features.** My earlier advice was animation next. With what you described, I would now put the interaction foundation first (11.3): it is what everything else on your list uses, and three of its four parts are already in phases 0 to 2.

---

## 13. Questions for you

These decide what I would recommend next. The first one matters most.

1. **The island (11.1).** Is it screen by screen with a transition at each edge, like the 2D Zelda games, or one map that scrolls without a break?
2. **Secret rooms.** When the player enters one, does the screen cut or fade to the room, and does the island keep running while the player is inside (enemies moving, timers), or does it freeze?
3. **"Another LDtk world."** Do you mean LDtk's multi-worlds feature inside one `.ldtk` file, or a separate `.ldtk` file per world? The first shares all definitions and is much simpler to support. The LDtk document explains the difference.
4. **Interaction.** Is it always "walk into it" as today, or also "stand next to it and press a key"? Does anything other than the player ever trigger things (an enemy stepping on a plate, a thrown object hitting a switch)?
5. **Combat.** Melee, projectiles or both? Does the player's attack have its own hitbox that appears for a few frames? That decides how soon "detect without moving" (11.4) is needed.
6. **Animation data (11.5).** Would a fixed layout on the sprite sheet be acceptable, or do you want to author animations in your editor?
7. **Persistence.** Should the world remember changes while the game runs (a chest stays open after you leave and come back)? Today it does, because every level stays loaded. That stops being free the moment levels are loaded on demand.
8. **Scale.** Roughly how many levels and how many entities per level do you have in mind? Tens and hundreds need no change to the current approach. Thousands do.
