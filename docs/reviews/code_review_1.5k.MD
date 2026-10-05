# Architecture Review #2 (Post-Refactor) — sc_game_engine

Scope: `engine/` + `very_cool_rpg/`. `vendor/` excluded. Focus: validating the boundary/state refactor.

## Verdict at a Glance

| Question | Result |
|---|---|
| Boundary severed (engine ⊘ LDtk)? | **Yes, in source.** Build config still leaks (see H2). |
| TextureCache DI safe? | **Yes** — but for a fragile, implicit reason. |
| projectDir DI safe? | **No — latent dangling reference (H1).** |
| Custom-field hashing bottleneck? | **Non-issue** — you aren't reading fields at all yet. |
| Locomotion/Kinematics split robust? | **Yes**, single-threaded, correctly ordered. Minor scaling caveats. |

The refactor achieved its main goal: the engine is genuinely LDtk-free in code, the God-struct was correctly avoided, and the texture cache is no longer an unbounded global leak. But the same constructor-injection that fixed the cache introduced a new dangling-reference trap via `projectDir`, and the `CharacterController` refactor removed a safety net.

---

## High Priority

### H1 — Dangling reference: `projectDir` is injected by reference to a stack local
The blueprints store `projectDir` **by reference**, bound to a variable that dies at the end of `OnAttach`:

- `GameplayLayer.cpp:56` — `fs::path projectDir = fs::absolute(ASSETS_DIR);` is a **local**.
- `GameplayLayer.cpp:81` — passed as `const fs::path&` into `RegisterAll`.
- `BlueprintRegistry.cpp:9-10` — forwarded as `const fs::path&` into `make_unique<PlayerBlueprint>(projDir, ...)`.
- `PlayerBlueprint.h:16` / `DragonBlueprint.h:16` — stored as `const fs::path& m_projectDir;`.

The blueprints live inside `m_entityFactory` (a `GameplayLayer` member) and therefore **outlive `OnAttach`**. The moment `m_projectDir` is read after `OnAttach` returns, it dereferences a destroyed stack object → undefined behavior.

Why it hasn't crashed: `Spawn()` is currently only called *inside* `OnAttach` (`GameplayLayer.cpp:82-85`), before the local dies. But the entire purpose of injecting into persistent blueprints is runtime spawning. The first `m_entityFactory.Spawn(...)` you call for a dynamic spawn (enemy wave, respawn, projectile) will run `Build()` → `AttachSprite(entity, ldtkData, m_projectDir, ...)` on a dangling path. This directly defeats the "safely injected during registration" intent.

**Fix — store the path by value.** Paths are small and copied exactly once at registration:
```cpp
// PlayerBlueprint.h / DragonBlueprint.h
private:
    fs::path      m_projectDir;   // by value — was: const fs::path&
    TextureCache& m_textureCache; // keep by reference (shared mutable cache)
```
Contrast with the cache (H-note below): the cache is a **layer member**, so a reference to it stays valid for the layer's whole life; `projectDir` is a **local**, so it must not be held by reference. (An even cleaner option: make `projectDir` a `GameplayLayer` member too, so both injections follow the same lifetime rule.)

### H2 — Boundary is enforced by discipline, not by the build
`grep` confirms **zero** `ldtk::` / `LDtkLoader` / `game::` references in `engine/` source — the code boundary is clean. But the build still hands the engine the tools to break it:

- `engine/src/pch.h:25` still `#include <nlohmann/json.hpp>` — its only consumer was the deleted `LDtkLoader.cpp`. Now completely unused (grep: no `nlohmann`/`json` usage anywhere in `engine/` except the pch line and the vcxproj).
- `engine.vcxproj:76,89` — `IncludePath` still lists `vendor\LDtkLoader\` and `vendor\nlohmann\`.

While those stay, any engine `.cpp` can `#include <LDtkLoader/Entity.hpp>` and compile fine, so nothing *structurally* prevents the leak from creeping back. Remove `<nlohmann/json.hpp>` from `engine/src/pch.h` and drop `vendor\LDtkLoader\` + `vendor\nlohmann\` from both `IncludePath` lines. After that, a re-introduced LDtk include in the engine won't compile — the boundary becomes mechanically enforced, which is what makes "the engine knows nothing about LDtk" actually true.

### H3 — `CharacterController` lost its default initializers → uninitialized-read footgun
The refactor stripped the in-class defaults:
```cpp
// GameComponents.h:8-13 (now)
struct CharacterController {
    float maxSpeed;      // was = 300.f
    float acceleration;  // was = 2500.f
    float friction;      // was = 2000.f
    sf::Vector2f direction{};
};
```
Today both blueprints pass `default_stats::CHARACTER_CONTROLLER` explicitly, so it works. But the type is now unsafe by default: `entity.AddComponent<components::CharacterController>();` (exactly what the *old* blueprints did, and the obvious thing to write for blueprint #3) yields **garbage** speed/accel/friction → an entity that flings off-screen or behaves nondeterministically. Either restore the in-class defaults (cheap insurance) or give `CharacterController` a constructor that requires the stats. Don't leave a movement-critical struct default-constructible into garbage.

---

## Medium Priority

### M1 — TextureCache DI is safe, but only by implicit member-ordering luck
You asked specifically. It **is** safe, for two reasons that are currently true but undocumented:

1. `m_textureCache` is declared (`GameplayLayer.h:22`) **before** `m_entityFactory` (`:27`). Members destruct in reverse order, so the factory (holding the blueprints that hold `TextureCache&`) dies **before** the cache. No dangling read during teardown.
2. Blueprints have trivial destructors that never touch the cache, so even out-of-order teardown wouldn't dereference it.

And because the cache is a layer *member* (not a local), runtime `Spawn()` calls are fine — unlike `projectDir` (H1). So: correct today. The fragility is that the safety is *invisible* — reorder those two member declarations, or add a blueprint whose destructor uses the cache, and it breaks silently with no compiler warning. Two options: (a) drop a comment on the members noting the ordering dependency, or (b) better, have `EntityFactory` own the `TextureCache` and hand blueprints a reference to *its* member — then factory-and-cache lifetimes are inseparable by construction.

### M2 — Stats still aren't data-driven; custom fields aren't read at all
Re: your performance question — there is no `getField<>()` call anywhere, so there is no string-hashing cost to worry about yet. Both `PlayerBlueprint.cpp:18` and `DragonBlueprint.cpp:17` assign the **same** `default_stats::CHARACTER_CONTROLLER`, so the Dragon still moves identically to the Player. The prior review's data-driven recommendation is half-done: literals moved into one shared struct, but nothing reads per-entity values.

When you do wire up LDtk custom fields (`ldtkData.getField<float>("maxSpeed")`, etc.), note that LDtkLoader resolves fields by **string name lookup per call**. That is completely fine here — spawning is one-time initialization, not a per-frame hot loop, so there is no bottleneck and **no need to pre-hash or cache field keys**. Only revisit if you ever move field reads into `OnUpdate`/`OnFixedUpdate` (you shouldn't).

### M3 — Window/camera aspect ratios don't match → sprites are stretched
`main.cpp:8` window = **1280×768** (5:3 ≈ 1.667); `GameplayLayer.h:23` camera view = **640×360** (16:9 ≈ 1.778). SFML scales the view to fill the window with a **non-uniform** factor (x: 1280/640 = 2.00, y: 768/360 = 2.133), stretching everything vertically by ~6.7%. This is a real visual defect, not just a magic-number smell. Pick a consistent pair — either window `1280×720` or view `640×384` — so you get a clean uniform 2× scale, and hoist the relationship into one named constant (e.g. `RENDER_SCALE` + base resolution) so the two can't drift apart again.

---

## Low Priority / Nitpicks

- **System ordering is an unenforced contract.** `OnFixedUpdate` (`GameplayLayer.cpp:97-103`) hand-orders input → locomotion → kinematics, which is correct. But when AI intent systems arrive they *must* slot in before `UpdateCharacterMovement`, with nothing to enforce it. Single-threaded, so no data races — this is purely an ordering-dependency risk as systems multiply. Keep the ordering centralized (as it is now) rather than scattering system calls.
- **`UpdateCharacterMovement` mutates the intent in place.** `character_movement.cpp:18-19` normalizes `controller.direction` and writes it back. Harmless for the player (reset every frame in `player_input.cpp:20`), but an AI that sets `direction` once and expects it to persist will find it silently renormalized. Treat `direction` as read-only input and compute the normalized vector into a local.
- **Dead code:**
  - `render_utils.cpp:8,11` — commented `//#include <components/SpriteComponent.h>` and `//namespace gcomp`.
  - `GameplayLayer.cpp:108-109` — commented `//trans.pos...`.
  - `player_input.cpp:19` — commented debug log.
  - `constants.h:5` (game) — `CREATE_CONST` macro is defined but never used in that file (`ASSETS_DIR` doesn't use it). Remove it there.
- **Unused includes:** `<format>` is in both `pch.h` files but `std::format` is never called — logging is all `printf`-family. Either drop it or (better) migrate logging to `std::format` for compile-time format checking.
- **Hardcoded level identifiers:** `"world_test1.ldtk"`, `"World_Level_0"`, and the empty world name `""` are literals in `GameplayLayer::OnAttach` (`:57,60,61`). Fine for one level; move to config before a second level-loading class copies them.
- **`ASSETS_DIR = "../../assets"`** (`constants.h:9`) is resolved relative to the working directory, which is brittle across run configurations. Consider deriving from the executable path.

---

## Regressions Fixed Since Review #1 (credit where due)
- ✅ `ldtk::Entity` no longer bleeds into engine headers — the factory/blueprint/assembler stack moved wholesale into `very_cool_rpg`.
- ✅ Dead `engine/loader/LDtkLoader.{h,cpp}` + the phantom `LevelGraphics.h` include are gone.
- ✅ Texture cache is a scoped `GameplayLayer` member (`m_textureCache`) instead of a never-invalidated global `s_cache`.
- ✅ `SpriteComponent`'s pointless template constructors are de-templated to plain `int16_t` (`Components.h:29-35`).

## Bottom Line
The boundary refactor is genuinely successful — this is now a properly layered engine/game split. Two things to fix before you spawn anything at runtime: **H1** (the `projectDir` dangling reference will bite the instant `Spawn()` is called outside `OnAttach`) and **H3** (the un-defaulted `CharacterController` will hand out garbage the instant a blueprint forgets to pass stats). **H2** is a 10-minute change that upgrades your boundary from "enforced by discipline" to "enforced by the compiler." Everything below that is polish.
