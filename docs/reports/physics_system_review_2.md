# Review: SpatialGrid/physics_system redesign + level/entity loading integration

Date: 2026-08-06
Scope: read-only review, no code changed. Follow-up to `physics_system_review.md` (2026-08-05),
reviewing the redesign in commit `791a111` and the currently staged changes on top of it
(`CellRange`, `Grid.hpp`, `SpatialGrid` rewrite, `physics_system` rewrite, `EntityFactory`/
`LevelLoader` wiring).

## TL;DR

All three bugs from the prior review are fixed, cleanly. The broad-phase pipeline (grid →
`Query` → `SweptAABB` → slide resolution) is now correct and re-syncs the grid every frame.
But the new dynamic-registration path — `EntityFactory::Spawn` → `RegisterEntityCollisions`
— is wired up in a way that **cannot work as designed**: it runs before the entity's collider
exists. It's currently harmless (nothing calls dynamic spawn yet, and level load has a second
safety-net pass that papers over it), but it's the exact kind of bug that will silently break
the first enemy spawner or projectile system you build, the same way the stale-grid bug did
last time. Worth fixing now, before it's buried under gameplay code that depends on it working.

## 1. Previously flagged bugs — all fixed

| # | Prior finding | Status |
|---|---|---|
| §1 | Grid built once at load, never updated as entities move | **Fixed.** `UpdateKinematics` (`physics_system.cpp:230-232`) calls `grid.MoveEntity` every frame for any entity whose position changed, gated by the same `IsCollidable` check used at insert time — the set-membership invariant is explicitly documented and honored. |
| §2 | `GetCellsFromBox` UB: negative float → `uint32_t` cast | **Fixed.** `SpatialGrid.cpp:114-135` now stays in float space through `std::clamp` and only casts to `uint32_t` after clamping to `[0, rows/cols]`. Comment at the call site correctly explains why (query bounds legitimately go negative when sweeping left/up). |
| §3 | `GetSingleHitbox` union takes max *size* instead of max *end point* | **Fixed** and renamed `GetEntityBounds` (`physics_system.cpp:21-40`): tracks `tl`/`br` independently and derives size as `br - tl`, the correct AABB-union approach. |

The `CellRange`/`Grid<Cell>` split is a good structural addition on top of the fix: `CellRange`
now owns `Intersects`/`Difference`, which `MoveEntity` uses to only erase/insert the
non-overlapping slice of cells when an entity shifts by less than a full cell — a real
optimization over naive erase-everything/insert-everything, and it reads cleanly because the
half-open-range convention is stated once in the header comment and consistently honored
everywhere (`< maxRow`, no off-by-ones spotted).

## 2. New finding: `RegisterEntityCollisions` runs before the entity has a collider

**The contract, as documented in `physics_system.h:9`:**
```cpp
// called when spawning a new entity dynamically after BuildSpatialGrid was already called
void RegisterEntityCollisions(math::SpatialGrid& grid, const Entity& entity);
```

**Where it's actually called** — unconditionally, inside `EntityFactory::Spawn`
(`EntityFactory.cpp:16-32`), immediately after `blueprint->Build(entity, ldtkData)`:

```cpp
it->second->Build(entity, ldtkData);
ecs::physics_system::RegisterEntityCollisions(level.GetSpatialGrid(), entity);
```

**The problem:** no blueprint (`PlayerBlueprint`, `DragonBlueprint`) adds a
`BoxColliderComponent`. Hitboxes come exclusively from LDtk tile custom data, applied by the
*caller* of `Spawn` — `EntityLayerBuilder::Build` — **after** `Spawn` returns:

```cpp
ecs::Entity entity = ctx.entityFactory.Spawn(*ctx.currentLevel, ctx.registry, ldtkEntity);
if (!customData.empty()) {
    sf::Vector2f pivotOffset = CalcPivotOffset(ldtkEntity);
    ApplyCustomData(entity, customData, pivotOffset);   // <- BoxColliderComponent added here
}
```

So at the moment `RegisterEntityCollisions` runs, the entity has a `TransformComponent` but no
`BoxColliderComponent` yet. It hits its own guard clause (`physics_system.cpp:194`), logs a
warning, and returns without inserting anything — for **every single entity** spawned through
the level loader, every load.

**Why it hasn't broken anything yet:** `LevelLoader::LoadLevel` calls `BuildSpatialGrid` once,
at the very end, after every layer (including all entities and their custom data) is fully
built. That second, independent full-registry sweep is the thing actually populating the grid
today — `RegisterEntityCollisions` is dead weight at load time, not a correctness bug, just
noise (a spurious `LOG_W` per entity per level load).

**Why it will break the first thing you build on top of it:** the doc comment says this
function exists for *dynamic* spawning — after `BuildSpatialGrid` already ran, so there's no
second sweep to bail you out. `EntityFactory::Spawn` is currently only called from
`EntityLayerBuilder` (confirmed via grep — no runtime spawner exists yet), so this hasn't been
exercised. But the moment you add one (enemy wave, projectile, pickup) and it follows the same
`Spawn` → `ApplyCustomData` pattern the loader already establishes as the idiom, that entity's
collider will silently never enter the grid. It'll never be found by `Query`, never block
movement, never register a hit — the same "looks like it works, fails invisibly" shape as last
review's stale-grid bug, just moved one layer up.

**Direction:** `EntityFactory::Spawn` shouldn't call `RegisterEntityCollisions` at all — it
returns before the entity is actually finished being built (custom data is still pending in the
caller). Move the call to whoever *does* know the entity is finished: have
`EntityLayerBuilder::Build` call it after `ApplyCustomData`, and require the future
dynamic-spawn path to do the same after its own setup. If you want `Spawn` itself to guarantee
"fully built" so callers can't forget, that likely means folding custom-data application into
`Spawn`/`IBlueprint::Build` rather than leaving it a separate post-step the caller has to
remember — worth deciding now while there's one call site, rather than after a second one
exists to keep in sync.

## 3. Minor, unchanged from last review

- `SpatialGrid::Query` (`SpatialGrid.cpp:66-83`) still allocates + sorts + dedupes a fresh
  vector per call, called up to `MAX_SLIDES` (3) times per moving entity per frame. The
  `results.reserve(32)` added this pass helps a little, but the allocation/sort pattern is
  unchanged. Still not worth fixing at today's entity counts — noting only so it isn't
  forgotten if it shows up in a profiler later.

## What's good

- The set-membership invariant between "who's in the grid" and "who gets `MoveEntity` calls"
  (`IsCollidable`, used identically at insert/register/move time) is stated once in a comment
  and actually held everywhere — this is exactly the kind of thing that quietly rots, and it
  hasn't here.
- `CellRange::Difference`'s 4-way split (rows above/below + cols left/right of the
  intersection) is a correct, minimal decomposition and is easy to verify by inspection because
  it's a pure function with no grid/entity state involved.
- `Grid<Cell>::TryGetCell` vs `GetCell` (nullptr-on-miss vs throw-on-miss) is a sensible split
  given `SpatialGrid` only ever calls the throwing variant with indices it just computed and
  clamped itself — a thrown out-of-range there means a real internal bug, not bad input.

## Priority order

1. Fix the `RegisterEntityCollisions` call-site ordering (§2) before building dynamic spawning
   or the debug system on top — the debug system will want to *trust* the grid to show what's
   really there, so this is worth closing first rather than debugging "why doesn't my spawned
   enemy collide" later without knowing this is why.
2. Everything else here is cosmetic/deferred (§3) — fine to move on to the debug tooling once
   §2 is handled.
