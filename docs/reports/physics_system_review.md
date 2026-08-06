# Review: `engine/.../physics_system.{h,cpp}` + `engine/math/{physics,SpatialGrid}.{h,cpp}`

Date: 2026-08-05
Scope: read-only review, no code changed. Follow-up to `proper_physics_system_explained.md`
(the spatial grid it recommended has since been built — this reviews that implementation).

## TL;DR

The broad-phase/narrow-phase/resolution pipeline is structurally correct and matches
the textbook Swept-AABB architecture — `SweptAABB` itself is a clean, faithful
implementation. But the spatial grid that was just added has a load-bearing gap: it's
built once at level load and **never updated as entities move**, which silently
breaks kinematic-vs-kinematic collision (two moving entities can't detect each other
correctly) rather than just "not implementing" it. There's also a real undefined-behavior
bug in the grid's cell-index math that triggers near the world origin. Ranked by impact
below.

## 1. The spatial grid goes stale the instant anything moves

`BuildSpatialGrid` (`physics_system.cpp:170`) is called exactly once, from
`LevelLoader.cpp:25`, right after a level finishes loading. It inserts **every**
entity with a `TransformComponent` + `BoxColliderComponent` — static rocks/walls
*and* dynamic entities (player, dragon, anything with `VelocityComponent`) — at
whatever position they spawned at.

`MoveAndResolve` (`physics_system.cpp:94`) then queries that grid every frame via
`grid.Query(queryBounds)`, but nothing ever calls `InsertEntity` again, and nothing
ever removes or re-buckets an entity that has moved to a new cell. Concretely:

- **Two moving entities never really collide.** Entity A queries the grid and finds
  entity B — but at B's *spawn* position, not wherever B has walked to since. As soon
  as B leaves its spawn cell, A can no longer detect it in the cells A actually queries
  (A's query bounds are computed from A's real position, but B is indexed under its
  stale one).
- **Ghosts at spawn points.** Conversely, A can spuriously "hit" a collision box at B's
  *original* spawn location even after B has walked away, because the grid still lists
  B there.
- The player is affected too — it's inserted into the grid at its LDtk spawn point like
  everything else, so other dynamic entities treat the player's spawn tile as
  permanently solid, forever, regardless of where the player actually is.

This isn't "kinematic-vs-kinematic isn't implemented yet" (that's a known, listed gap
in the prior report) — it's worse: it *looks* like it might work, will pass casual
testing if nothing happens to be near another dynamic entity's spawn point, and then
fails in a confusing, position-dependent way once the level fills up with moving
things. Worth fixing before building anything on top of it (pushing, aggro radii,
projectiles), since those will all inherit this bug silently.

**Direction:** the grid needs a lifecycle op beyond "build once" — either re-insert
each dynamic entity's current AABB into the grid at the *start* of `UpdateKinematics`
each frame (clear-and-rebuild for dynamics only, statics can stay put since they never
move), or track each entity's last cell and only re-bucket it when it crosses a cell
boundary. Given cell counts here are small, clear+rebuild-for-dynamics-only is the
simpler correct option and probably not a perf concern (see §4).

## 2. `GetCellsFromBox` has undefined behavior for any box near/left of the world origin

`SpatialGrid.cpp:37-51`:

```cpp
K topLeftCell = {
    .row = (uint32_t)std::floor(topLeftPos.y / m_cellSize),
    .col = (uint32_t)std::floor(topLeftPos.x / m_cellSize)
};
```

If `topLeftPos.x` (or `.y`) is negative — which happens routinely, since
`MoveAndResolve`'s query bounds (`physics_system.cpp:121-130`) are deliberately
extended backward via `std::min(0.0f, delta.x)` for anything moving left or up —
`floor(negative / cellSize)` produces a negative float, and casting a negative float
to `uint32_t` is **undefined behavior** in C++ (not just wraparound — the value isn't
representable in the target type at all). In practice on MSVC this usually truncates
to a huge value near `UINT32_MAX`, which then flows into:

```cpp
uint32_t xCells = bottomRightCell.col - topLeftCell.col;
```

producing either a nonsense cell count (multi-billion-iteration loop → hang/OOM) or,
depending on exact values, a silently wrong small number. This will reproduce
reliably for a player standing near `x=0` or `y=0` (a very normal spawn/corner
position) and moving left/up. This is the kind of bug that won't show up near the
middle of a level and then hard-locks the game the moment a player walks toward the
top-left corner of the map.

**Direction:** clamp the query/insert rect to the grid's valid bounds before
converting to cell indices, or make `GridCell` use signed integers (`int32_t`) and
only reject/clamp at the hash/lookup boundary. Signed indices are the more robust fix
since "moving toward negative space" is a completely normal thing for an AABB sweep
to compute even if the level itself starts at `(0,0)`.

## 3. `GetSingleHitbox` merges hitboxes incorrectly for multi-hitbox entities

`physics_system.cpp:11-24` builds one bounding hitbox from a collider's hitbox list
by taking the min offset and the **max size** independently:

```cpp
singleHitbox.offset.x = std::min(singleHitbox.offset.x, hb.offset.x);
singleHitbox.size.x = std::max(singleHitbox.size.x, hb.size.x);
```

That's only correct if the hitbox with the smallest offset also happens to be the
widest one. For two disjoint hitboxes — e.g. offset `(0,0)` size `(10,10)` and offset
`(20,0)` size `(5,5)` — the true encompassing box needs to reach `x=25`, but this
produces offset `(0,0)` size `(10,10)`, i.e. it doesn't cover the second hitbox at
all. This is only used to compute the box inserted into the grid at load time
(`physics_system.cpp:176`), but for any entity with more than one hitbox that isn't
roughly concentric, the entity will register as smaller than it actually is and can
be missed by broad-phase queries from the far side.

**Direction:** compute min offset and max *end point* (`offset + size`) separately,
then derive size as `maxEnd - minOffset`. Standard AABB union, not size-max.

## 4. Query-side allocation churn (minor, but worth naming)

`SpatialGrid::Query` (`SpatialGrid.cpp:15`) allocates a fresh `std::vector`, then
`std::sort` + `unique`-erases it, on every call — and `MoveAndResolve` calls it up to
`MAX_SLIDES` (3) times per moving entity per frame. For today's entity counts this is
irrelevant, but it's the standard "hidden allocation in a hot loop" pattern that AAA
physics code avoids (thread-local scratch buffer, or a small inline/SBO vector). Not
worth fixing now; worth remembering if entity counts grow or this becomes profiler-visible.
Similarly, `Query` calls `m_grid.contains(cell)` then `m_grid[cell]`
(`SpatialGrid.cpp:20-21`), which is two hash lookups where `find()` once would do.

## 5. What's actually good here

Worth saying explicitly, since the rest of this is mostly gaps:

- **`SweptAABB` (`physics.cpp`) is a correct, textbook implementation** — expanded
  target rect, ray-vs-AABB slab test, zero-velocity axis handling, and the
  already-overlapping-at-frame-start case are all handled properly. This is the hard
  part of the pipeline and it's solid.
- **The slide-resolution loop** (`MoveAndResolve`, up to `MAX_SLIDES`) correctly
  implements the "move to impact, zero the blocked axis, consume remaining time,
  repeat" pattern used by real 2D swept engines (Celeste-style). Corner cases (hitting
  a corner and sliding along the second surface) are handled by the loop structure,
  not special-cased, which is the right way to do it.
- **Separation of concerns is respected**: `character_movement.cpp` owns acceleration/
  friction/max-speed and only ever writes `VelocityComponent`; `physics_system` never
  reads game-level components and only ever reads/writes `TransformComponent` +
  `VelocityComponent`. This matches the "physics doesn't know what a game is" principle
  from the prior report and means gameplay code can be reworked without touching
  physics.
- **Fixed-timestep decoupling** (`Application::Run`, accumulator pattern) is correct
  and standard. One adjacent (not physics-file) risk worth flagging: there's no cap on
  how many fixed steps can run per frame, so a stall (breakpoint, asset load hitch) will
  make the accumulator dump many steps in a row on the next frame — the classic
  "spiral of death." A `while` with a max-steps-per-frame guard is the usual fix; not
  urgent, but it directly affects physics stability so it belongs on the same list.

## 6. Where this sits vs. a standard pipeline

Per the phase breakdown in `proper_physics_system_explained.md`:

| Phase | Status |
|---|---|
| Integration | done (`character_movement` → `VelocityComponent`) |
| Broad phase | **implemented but broken** — grid exists, is stale for anything that moves (§1) |
| Narrow phase | done, correct (`SweptAABB`) |
| Resolution (sliding) | done, correct (`MoveAndResolve` loop) |
| Kinematic vs Kinematic | not implemented (blocked by §1 — fix the grid first, this is likely "free" once dynamics re-insert per frame) |
| Triggers / sensors | not implemented |
| Collision events → gameplay | not implemented (no `CollisionEvent` dispatch yet) |

## Suggested priority order (not a mandate)

1. Fix the grid staleness (§1) — everything else, including kinematic-vs-kinematic,
   is straightforward once dynamic entities are re-bucketed per frame.
2. Fix the negative-coordinate UB in `GetCellsFromBox` (§2) — small, isolated, and a
   real crash/hang risk near world origin.
3. Fix `GetSingleHitbox`'s AABB union math (§3) — small, isolated.
4. Add a `CollisionEvent` dispatch (EnTT signals) once §1 is solid, so gameplay
   (combat, triggers, doors) can subscribe instead of the physics system needing to
   know about game concepts.
5. Query allocation churn (§4) and the fixed-step spiral-of-death guard — low priority,
   revisit if profiling or a stress test surfaces them.
