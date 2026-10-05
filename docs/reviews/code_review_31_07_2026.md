# Code Review — sc_game_engine

**Date:** 2026-07-31
**Scope:** `engine/` (the reusable engine library) and `very_cool_rpg/` (the game built on top of it). `vendor/` was excluded per request. No code was modified.

**Overall size:** ~30 engine source/header files, ~25 game source/header files. This is a small, early-stage codebase, so most findings are about setting good precedents now rather than fixing rot.

---

## 1. Engine (`engine/`)

### 1.1 Architecture overview

The engine is organized into clear subsystems: `core` (Application/Window/ILayer), `ecs` (EnTT wrapper + two systems), `graphics` (Texture2D/TileSet/Camera2D), `renderer` (a free-function `render2d` batching module), `world` (Level/Layer hierarchy), `input`, and `utils/logging`. The layering is sound at a glance — `core` doesn't know about `world`, `ecs` doesn't know about `renderer` details beyond calling into it, etc. Naming is consistent (`sc::` root namespace, PascalCase types, `m_` member prefix). For a project at this stage, the skeleton is genuinely well thought out.

The two biggest structural themes worth calling out:

### 1.2 Global mutable state in "subsystem" modules

`render2d` (`engine/src/renderer/render2d.cpp`) and `input` (`engine/src/input/Input.cpp`) are implemented as namespaces of free functions backed by file-static globals (`s_quadQueue`, `s_renderTarget`, `s_contexts`, `s_initialized`, etc.) rather than as objects owned by `Application`/`Window`.

- This makes the engine implicitly a singleton: two `Application`/`Window` instances (e.g. a secondary editor window, an offscreen renderer, or just running tests in the same process) are not possible.
- It also makes call-order a hidden contract enforced only by `assert(s_initialized)` — which disappears entirely in release/NDEBUG builds (see 1.4). A `render2d::SubmitQuad` called before `Initialize()` in a release build is silent undefined behavior rather than a caught error.
- `input::IsActionActive` degrades to `LOG_W` + `return false` when no context is pushed, which is reasonable, but note the asymmetry with `render2d`'s hard asserts for the same class of "used it before it was initialized" mistake — the two modules disagree on how to fail.

**Recommendation:** at minimum, wrap `render2d`'s state in a small class owned by `Application` (or `Window`) so lifetime is explicit and the assumption "only one renderer per process" becomes structurally true rather than implicit.

### 1.3 Layer rendering is done via manual RTTI, not polymorphism

`ILevelLayer` carries a `LayerType` enum, and `render_system::RenderWorld` (`engine/src/ecs/systems/render_system.cpp:29-59`) branches on that enum and `static_cast`s to the concrete layer type:

```cpp
if (layer->GetType() == world::LayerType::Tile) {
    auto* tileLayer = static_cast<world::TileLayer*>(layer.get());
    render2d::DrawTileLayer(*tileLayer);
}
else if (layer->GetType() == world::LayerType::YSortedTile) { ... }
else if (layer->GetType() == world::LayerType::Entity) { ... }
else if (layer->GetType() == world::LayerType::Collision) { /* TODO */ }
```

This is a classic open/closed violation: every new layer type requires editing this `if/else` chain, the `LayerType` enum, *and* remembering to update `IsDepthBoundary()` semantics for that type. The `static_cast` is also unchecked — if `GetType()` and the actual dynamic type ever disagree (easy to introduce by copy-pasting a constructor and forgetting to change the type argument), this is silent memory corruption rather than a caught bug.

**Recommendation:** give `ILevelLayer` a virtual `Draw(...)` (or accept a visitor) so each layer type owns its own rendering dispatch, and the renderer just iterates and calls a virtual — no enum/cast pair to keep in sync.

### 1.4 Asserts as the only safety net, and they vanish in Release

`assert()` is the primary invariant-checking mechanism throughout (`Entity::AddComponent/GetComponent/HasComponent`, all of `render2d`, `logging::LogOutput`'s buffer bound check). In an NDEBUG/Release build these compile out entirely. That means:

- `Entity` used after its `Registry*` is stale, or with a null handle, is checked-in-debug/UB-in-release.
- `render2d` functions called out of order (e.g. `SubmitQuad` without a `BeginBatch`) are checked-in-debug/UB-in-release.
- `logging.cpp:113`'s `assert(end < spacesBuff + sizeof(spacesBuff))` guards a fixed 128-byte stack buffer against a large `g_IndentLevel` — in Release this bound check disappears and a sufficiently deep `LOG_SCOPE()` nesting (e.g. runaway recursion) becomes a stack buffer overflow instead of a caught bug. This one is worth prioritizing since logging code runs unconditionally and indent depth is caller-controlled.

**Recommendation:** for programmer-error contracts that are cheap to check, consider keeping the check active in Release too (a lightweight `SC_VERIFY`-style macro that always executes, vs. `assert` which is Debug-only), at least for the logging buffer bound and any renderer-order checks that touch memory.

### 1.5 `CollisionLayer::IsSolid` is declared but never defined

`engine/include/engine/world/CollisionLayer.h:21` declares `bool IsSolid(float pixelX, float pixelY) const;` — there is no `CollisionLayer.cpp` anywhere in the tree. Nothing currently calls it, so the missing definition hasn't caused a link error yet, but the moment any code calls `IsSolid`, the build breaks. More broadly, this reflects that **collision is a stub end-to-end**: `world_loader.cpp` parses LDtk collision layers into `CollisionLayer` grids (`ProcessCollisionLayer`), but nothing in `physics_system` or anywhere else ever queries that data. The physics system (`UpdateKinematics`) is pure unconstrained Euler integration — entities can walk through anything. This is fine as a work-in-progress state, but it's worth tracking explicitly (e.g. a TODO or issue) rather than leaving it as an implicit gap, since it's easy to forget the loader and the query side were never wired together.

### 1.6 Logging system

`logging.h`/`logging.cpp` is a reasonable printf-style logger with level filtering, optional file/console/time/filename output, and a scoped-indent helper (`ScopedLog`). A few notes:

- `LogOutputRaw(spacesBuff)` (`logging.cpp:118`) passes a caller-influenced-length buffer directly as the *format string* argument rather than `LogOutputRaw("%s", spacesBuff)`. In this specific case `spacesBuff` only ever contains spaces so there's no live vulnerability, but the pattern (variable data as format string) is the same shape as a format-string bug and is worth avoiding on principle — a future edit that puts other content into that buffer would introduce one silently.
- `__RELATIVE_FILE__` (`logging.h:4`) computes a relative path by pointer arithmetic on `__FILE__`, assuming every compiled file lives under `<ProjectDir>src\`. This holds today, but there's no bounds check — a file compiled from outside that directory (a generated file, a file added at the project root, or the engine's headers being compiled under `very_cool_rpg`'s `PROJECT_DIR` instead of `engine`'s) would produce a pointer past the start of the string, and the log line would print garbage or crash. Since it's resolved as `__FILE__ + N`, this is a silent OOB read risk if the assumption is ever violated.
- The `operator new`/`operator delete` tracked-allocation override (`logging.cpp:9-13`) is gated behind `SPD_ENABLE_TRACKED_ALLOC == 1` and calls `SPD_ALLOC`/`SPD_FREE`, but neither macro is defined anywhere in the repo (checked both `.vcxproj` preprocessor definitions and all headers). This code is currently dead (the `#if` evaluates false since the macro is undefined), but it's a leftover from a template/another project — either wire it up or remove it so a reader doesn't have to go hunting for `SPD_ALLOC`'s definition.
- `ADD_CLASS_TAG` (`logging.h:94-97`) is a macro that injects `public:`/`protected:` access-specifier changes into whatever class body it's pasted into, with a comment warning "should always add at beginning or end to not mess up access modifiers." This works, but it's a fragile code-generation pattern — a class member added after the macro without noticing the trailing `public:` it left behind changes visibility silently. A small `Tagged` CRTP/base-class mixin would give the same `SetTag`/`m_tag` functionality without mutating access state at the call site.

### 1.7 Minor items

- `Camera2D::GetSize()`/`GetHalfSize()` return `const sf::Vector2f` by value (`Camera2D.h:15-16`) — the top-level `const` on a return-by-value type is a no-op in C++ (it only affects use as an rvalue and blocks calling non-const methods on the temporary); harmless but adds noise.
- `Window::Update()` only handles `sf::Event::Closed` (`Window.cpp:13-22`). SFML windows are resizable by default and no style flags restrict that here, but there's no resize handling — resizing the window will not update the view/camera, causing stretched or clipped rendering. Either disable resizing explicitly (`sf::Style` / `sf::State`) or handle `Resized` events.
- `Application::Run()` advances physics on a fixed accumulator (correct pattern) but renders with zero interpolation between the last two physics states — at low frame rates or with `PHYSICS_HZ` mismatched to `MAX_FPS`, motion will visibly stutter. Not a bug, just worth knowing if smoothness becomes a priority later.
- `TileSet` (`TileSet.cpp:6-32`) silently truncates when `tileSize` doesn't evenly divide the texture's width/height (integer division into `m_cols`/`m_rows`) — a mis-sized tileset asset would silently drop a partial row/column of tiles rather than warning.
- `World::GetLevel(name)` does a linear scan (`World.cpp:9-16`) while `GetLevel(uid)` is a hash lookup — inconsistent lookup cost for what's conceptually the same operation. Not a real problem at current scale (level counts are small), but worth knowing if it's called per-frame anywhere later.

### 1.8 What's good here

- The `ILayer`/`Application` split (fixed update / variable update / render, layer push/pop) is a clean, standard game-loop shape and easy to extend.
- `Entity` as a thin EnTT handle wrapper is idiomatic and keeps `entt` mostly out of game code.
- `render2d`'s Y-sort batching (`FlushQuads`, depth-boundary flushing keyed off `ILevelLayer::IsDepthBoundary()`) is a genuinely nice piece of design — it lets tile layers and dynamically Y-sorted content (entities + Y-sorted tiles) interleave correctly without the renderer needing to know about game-specific concepts. This is the strongest part of the engine.
- Precomputing/reserving `PREALLOCATED_QUAD_QUEUE` and using a single reused `sf::Sprite` (`s_sceneSprite`) for the whole batch avoids per-quad allocation — good attention to per-frame cost this early.

---

## 2. Game (`very_cool_rpg/`)

### 2.1 Architecture overview

The game layers cleanly on top of the engine via a single `GameplayLayer`, an `EntityFactory` + `IBlueprint`-per-entity-type pattern for turning LDtk entity definitions into ECS entities, and small free-function systems (`player_input`, `character_movement`) that run alongside the engine's own `physics_system`. This is a sensible, data-driven shape for a small RPG and mirrors patterns from established engines (blueprint/prefab registries, per-frame system functions operating on EnTT views).

### 2.2 `GameplayLayer` has leftover dead code and silent failure paths

`GameplayLayer::OnFixedUpdate` (`GameplayLayer.cpp:57-82`) has commented-out lines (`//trans.pos.x += 1.f;`, and the entire camera-clamp-to-map-edges block) sitting in committed code. Either implement the map-edge clamping or delete the commented block — as-is it reads as unfinished work with no indication of whether it's planned or abandoned.

Separately, the player lookup in `OnAttach` (`GameplayLayer.cpp:49-54`):

```cpp
auto view = m_registry.GetRegistry().view<components::PlayerTag>();
for (auto entityHandle : view) {
    m_player = sc::ecs::Entity(entityHandle, &m_registry);
    break;
}
```

silently leaves `m_player` as a default (null) `Entity` if no `PlayerTag` entity was spawned (e.g. blueprint registration typo, or the LDtk level has no Player entity). The only downstream effect is the camera never updates (`OnFixedUpdate` guards with `if (m_player && ...)`), which will look like "the camera is broken" with no log line pointing at the actual cause. A `LOG_W` when no player entity is found would save real debugging time later.

### 2.3 Hardcoded level name

`GameplayLayer::OnRender` (`GameplayLayer.cpp:85`) hardcodes `"World_Level_0"` as the level to render — there is no level-switching mechanism yet. Fine for a single-level prototype, but this is the one string a future "load level 2" feature will have to touch, so it's worth keeping in mind rather than something to fix now.

### 2.4 Blueprint duplication and central registration

`PlayerBlueprint::Build` and `DragonBlueprint::Build` (`PlayerBlueprint.cpp`, `DragonBlueprint.cpp`) are identical except `PlayerBlueprint` additionally adds `PlayerTag`:

```cpp
utils::assemblers::AttachTransform(entity, ldtkData);
utils::assemblers::AttachSprite(entity, ldtkData, m_projectDir, m_textureCache);
entity.AddComponent<ecs::VelocityComponent>();
entity.AddComponent<components::CharacterController>();
```

Two blueprints is too early to be sure this is a real problem (rule of three), but it's worth watching — if a third or fourth blueprint repeats the same four lines, pull them into a shared `AttachCommonMovable(entity, ldtkData, projectDir, cache)` helper in the assemblers namespace, called by each blueprint before its type-specific additions.

Also worth noting: `DragonBlueprint` attaches `CharacterController`, but nothing currently drives `direction` for non-player entities (only `player_input::UpdatePlayerInput` sets `direction`, and it's scoped to `PlayerTag` entities). So the Dragon's `CharacterController` is inert — it has acceleration/friction/maxSpeed but `direction` stays `{0,0}` forever, meaning the component is currently doing nothing for that entity. Not wrong, just worth being aware it's a placeholder for future AI-driven movement rather than functioning today.

`BlueprintRegistry::RegisterAll` (`BlueprintRegistry.cpp:8-11`) hardcodes the full list of entity-type strings ("Player", "Dragon") in one place that has to be kept in sync with LDtk entity definition names by hand. At two entity types this is totally reasonable; if the roster grows significantly, a self-registration mechanism (each blueprint registers itself via a static initializer keyed by its own name) removes the need to touch this file per new entity type. Not urgent at current scale.

### 2.5 `EntityFactory::Spawn` on unknown blueprint

`EntityFactory::Spawn` (`EntityFactory.cpp:15-28`) logs a warning and returns an entity that has *only* the default `TagComponent` when no blueprint is registered for an LDtk entity name. This entity persists in the registry with no `TransformComponent`/`SpriteComponent` — harmless today since nothing iterates "all entities," but any future system that assumes all entities have a transform (e.g. a spatial partition, a debug overlay) would need to guard against this. Consider whether an unmatched blueprint should skip entity creation entirely rather than leaving a stub.

### 2.6 `CharacterController` mixes tuning data with per-frame state

`GameComponents.h`:

```cpp
struct CharacterController {
    float maxSpeed = 250.f;
    float acceleration = 2000.f;
    float friction = 5000.f;
    sf::Vector2f direction{}; // -1 to 1 on X and Y
};
```

`maxSpeed`/`acceleration`/`friction` are per-entity-type configuration (set once at blueprint time), while `direction` is mutated every fixed-update by `player_input`. Bundling config and mutable per-frame input state in one component works fine now, but as more input sources appear (AI-driven `direction` for Dragon, scripted cutscene movement, etc.) it'll be easier to reason about if "movement tuning" and "current input/intent" are two separate components. Flagging as a design note, not a bug.

### 2.7 Path handling is CWD-relative

`constants.h`: `const fs::path ASSETS_DIR = fs::path("..") / ".." / "assets";` is relative and depends on the process's current working directory at launch, not on the executable's location. This works as long as the binary is always run from the same build-output directory (true today, given the Visual Studio project setup), but it will silently break (LDtk project "doesn't exist" from `world_loader::Load`'s own check) the moment the exe is launched from a different CWD — e.g. double-clicking it in Explorer, running it via a shortcut, or packaging it for distribution. Resolving assets relative to `argv[0]`'s directory (or an executable-relative path query) would remove this fragility.

### 2.8 Minor items

- `player_input.cpp:19` has a commented-out debug log (`//LOG_D(...)`) left in place — either remove or leave as an intentional "uncomment to debug" note (harmless either way, just noting it alongside the other commented-out block in `GameplayLayer`).
- Namespace-alias style is inconsistent across files — some `.cpp` files do `namespace ecs = sc::ecs;` at file scope, others do `using namespace sc::graphics;`. Both are fine individually, but picking one convention project-wide (namespace aliases are generally safer than `using namespace` for anything beyond a tiny scope) would make the codebase easier to skim.
- No automated tests exist anywhere in the project (engine or game). Understandable at this stage, but the ECS systems (`character_movement`'s acceleration/friction/clamp math in particular) are pure, side-effect-free functions over data and would be cheap to unit test without needing a window/renderer — a good first target if testing is ever prioritized.

### 2.9 What's good here

- The blueprint/factory pattern cleanly separates "how do I turn LDtk data into an ECS entity" from both the LDtk-parsing code and the engine itself — `world_loader` doesn't know what a Player or Dragon is, it just calls `entityFactory.Spawn(...)`. This is the right level of indirection for a data-driven entity system and will scale well as more entity types are added.
- `character_movement.cpp`'s acceleration/friction/max-speed handling is correct, readable, and appropriately guards divide-by-zero (`if (currentSpeed > 0.0f)` before dividing).
- Keeping `player_input` (reads `sc::input`, writes `CharacterController::direction`) and `character_movement` (reads `CharacterController`, writes `VelocityComponent`) as separate systems is a nice, ECS-idiomatic separation between "intent" and "physics response," and makes it easy to later swap `player_input` for an AI-driven equivalent without touching movement math.

---

## 3. Summary

| Area | Engine | Game |
|---|---|---|
| Organization | Clear subsystem boundaries, consistent naming | Clean layering on engine, sensible blueprint/factory split |
| Biggest structural risk | Global singleton-style state in `render2d`/`input`; manual RTTI dispatch in `render_system` | Collision system is a stub end-to-end (loaded, never queried) |
| Biggest correctness risk | Asserts (Debug-only) as the sole guard for renderer call-order and the logging indent buffer bound | CWD-relative asset path; silent no-player-found failure |
| Lowest-effort high-value fix | Implement or remove `CollisionLayer::IsSolid`; fix `LogOutputRaw(spacesBuff)` to `"%s"` | Remove dead commented-out code in `GameplayLayer`; log when no player entity is found |

Nothing found here is severe for a project at this stage — the foundational architecture (layers, ECS via EnTT, Y-sorted batched rendering, blueprint-driven entity spawning) is solid and idiomatic. The recurring theme is **implicit contracts that aren't enforced by the type system**: layer type ↔ static_cast, `ADD_CLASS_TAG` ↔ `LOG_OBJ` macros, singleton renderer/input state ↔ call order, Debug-only asserts ↔ release safety. None of these need fixing today, but they're the places most likely to produce a confusing bug once the codebase grows past what one person can hold in their head.
