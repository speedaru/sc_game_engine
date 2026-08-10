# Tile collision & physics pipeline plan

**Date:** 2026-08-07 (rev. 2 — decisions folded in)
**Scope:** `engine/world/CollisionLayer`, `engine/math/SpatialGrid`, `engine/ecs/systems/physics_system`, and the game-side baking step

> **Rev. 2 changes.** Four questions were answered and the design moved with them:
> mixed authoring resolutions are now a first-class requirement (§3, §5) rather than a constraint to
> enforce; `CollisionLayer` drops `ILevelLayer` (§4); `math::Hitbox` is already relocated (done);
> runtime tile mutation and tile identity are in phase 1 (§8).

---

## 1. Where we are

Today there is exactly **one** collision pipeline, and everything is forced through it:

```
solid tile ──► ECS entity (Tag + Transform + BoxCollider) ──┐
                                                            ├──► SpatialGrid ──► Query ──► narrow phase
dragon/player ──► ECS entity (Tag + Transform + BoxCollider)┘
```

`TileLayerBuilder::SpawnTileCollider` creates one full ECS entity per solid tile. That is wrong on
four separate axes.

### 1.1 Cost

For a 100×100 tile level with ~50% solid coverage — 5000 tile entities:

| | per tile | 5000 tiles |
|---|---|---|
| `entt` entity + sparse-set slots | ~16 B | 80 KB |
| `TagComponent` — `std::format("Tile_Collider_{}_{}")` | string + **1 heap alloc** | **5000 allocs** |
| `TransformComponent` | 16 B | 80 KB |
| `BoxColliderComponent` — `vector<Hitbox>` | 24 B + **1 heap alloc** | **5000 allocs** |
| `SpatialGrid::m_entityCells` entry | ~40 B hashed | 200 KB |
| `SpatialGrid` cell vectors | handle repeated per covered cell | ~5000+ handles |

**~10 000 heap allocations and ~0.5 MB to express 5000 booleans.** The same information as a
shape-index grid is `5000 × 2 B = 10 KB` in **one** allocation.

### 1.2 The broad phase does redundant work

A tile grid *is already* a spatial acceleration structure. Feeding tiles into a second one
(`SpatialGrid`) throws away the free O(1) lookup and replaces it with a hash lookup, a per-cell vector
walk, a `std::sort` + `std::unique`, and a heap allocation per query — to rediscover something
`floor(x / cellSize)` answers exactly.

### 1.3 The narrow phase is cache-hostile

`FindClosestCollision` receives `vector<entt::entity>` and does **three sparse-set lookups per
candidate** (`all_of`, `get<Transform>`, `get<BoxCollider>`), each a random access into a different
storage pool, then chases `BoxColliderComponent`'s heap-allocated vector. For a player standing on
the ground that is ~16 candidates × 3 random lookups × a pointer chase, every slide iteration, every
fixed step.

### 1.4 It's conceptually wrong

Tiles have no identity, no lifetime, no components, and never move. Giving them entity handles means
every ECS view in the engine walks past thousands of things that are not entities, and every debug
statistic ("how many entities exist?") is meaningless.

### 1.5 The existing `CollisionLayer` stub has the wrong interface

```cpp
bool IsSolid(float pixelX, float pixelY) const;   // point query
```

A **point query is unusable for swept AABB.** You cannot derive a contact time or a normal from it,
and sampling points to approximate a box tunnels through thin geometry. The API has to yield the
actual overlapping rects. (The stub is currently mid-edit and doesn't compile — `ILevelLayer` and
`LayerId` are gone from its includes but still referenced in the constructor. §4 replaces it wholesale.)

---

## 2. Target architecture

The industry-standard shape (Godot `TileMap` physics, Unity `TilemapCollider2D`, Box2D static chains,
and essentially every 2D platformer since the 80s) is:

> **Two broad phases, one narrow phase.**
> Static tile geometry gets a *direct-indexed grid* — the grid is its own acceleration structure.
> Dynamic and free-standing entities get a *spatial hash*. Both emit candidate rects into a shared
> buffer, and the narrow phase never learns where a candidate came from.

```
                          ┌─────────────────────────────────────┐
   query AABB ──────────► │ CollisionLayer::QueryArea()         │  O(1) index, no alloc
   (swept bounds)         │   cell range → shape palette → rects│
                          └──────────────┬──────────────────────┘
                                         │  appends
                                         ▼
                          ┌─────────────────────────────────────┐
                          │      CandidateBuffer                │  reused, allocation-free
                          │      vector<ColliderRef>            │  after warmup
                          └──────────────▲──────────────────────┘
                                         │  appends
                          ┌──────────────┴──────────────────────┐
   query AABB ──────────► │ SpatialGrid::Query()                │  entities only now
                          │   cells → handles → resolved rects  │
                          └─────────────────────────────────────┘
                                         │
                                         ▼
                            physics_system narrow phase
                            flat loop of SweptAABB over rects
```

The critical property: **the narrow phase becomes a tight loop over a contiguous array of
`FloatRect`s.** No registry lookups, no pointer chasing, no branching on source. That is where the
real speedup is, more than the memory saving.

---

## 3. Data design

### 3.1 Shape palette + shape-index grid

Cells don't need a hitbox list each; they need a *reference* to one of the handful of distinct shapes
in play. A 200-tile tileset typically has 3–5 distinct collision shapes: full, empty, half-height, a
couple of quarter boxes.

```cpp
namespace sc::world {
    // cell-local hitboxes, relative to the cell's top-left corner.
    // INVARIANT: every box is fully contained within the cell (see 3.3)
    struct TileShape {
        std::vector<math::Hitbox> boxes;
        uint32_t flags = 0;    // reserved: OneWay, Trigger, material id. see 3.4
    };

    class CollisionLayer {
    public:
        static constexpr uint16_t EMPTY_SHAPE = 0;    // index 0 is always the empty shape
        static constexpr uint32_t INVALID_CELL = ~0u;

        // appends every hitbox overlapping `area` to `out`, in world space.
        // does NOT clear `out` - both broad phases append to the same buffer
        void QueryArea(const sf::FloatRect& area, physics::CandidateBuffer& out) const;

        // runtime mutation: destructible terrain, doors, pressure plates (see 3.4)
        uint16_t GetCell(uint32_t row, uint32_t col) const;
        void     SetCell(uint32_t row, uint32_t col, uint16_t shapeIndex);

        const TileShape& GetShape(uint16_t index) const;
        uint32_t CellIndex(uint32_t row, uint32_t col) const { return row * m_size.cols + col; }

        uint32_t GetCellSize() const { return m_cellSize; }   // for the debug overlay
        math::GridSize GetSize() const { return m_size; }

    private:
        sf::Vector2f   m_origin;       // world position of cell (0,0); LDtk layers can be offset
        uint32_t       m_cellSize;
        math::GridSize m_size;

        std::vector<uint16_t>  m_cells;    // rows*cols, index into m_shapes
        std::vector<TileShape> m_shapes;   // deduped palette; m_shapes[0] is empty
    };
}
```

`QueryArea` is pure arithmetic — clamp the area to a cell range, walk it row-major, skip
`EMPTY_SHAPE`, emit `box.offset + cellOrigin`. No allocation, no dedup (each cell is visited once),
no hashing.

For a 20×28 px player box swept by a frame's movement, that is **6–12 array reads**, against the
current path's hash lookup + vector walk + sort + unique + ~16×3 sparse-set lookups.

### 3.2 Mixed authoring resolutions — the thing that shapes everything else

You want **16 px tile custom data *and* an 8 px IntGrid** (already added to `world_test1.ldtk`), plus
possibly hand-drawn rects later. Three different resolutions feeding one grid.

The invariant that makes `QueryArea` correct is **"a cell's boxes are contained within that cell"** —
otherwise a query whose range excludes a box's *owning* cell would miss geometry that overlaps the
query. Everything follows from preserving that invariant while accepting arbitrary input.

**The builder exposes exactly one primitive, and every source goes through it:**

```cpp
class CollisionLayerBuilder {
public:
    CollisionLayerBuilder(sf::Vector2f origin, uint32_t cellSize, math::GridSize size);

    // clips `worldBox` to each cell it overlaps and appends the fragment to that cell.
    // this is the ONLY way geometry enters. sources of any resolution just work
    void AddBox(const sf::FloatRect& worldBox, uint32_t flags = 0);

    CollisionLayer Bake();   // normalize per cell, dedup into palette, write indices

private:
    std::vector<std::vector<math::Hitbox>> m_pending;   // per cell, pre-dedup
};
```

- **16 px tile, custom-data hitbox** → `AddBox(tileOrigin + hitbox)`. Contained in one 16 px cell.
- **8 px IntGrid value** → `AddBox(cellRect)`. Nests cleanly as a quadrant of a 16 px cell.
- **Arbitrary hand-drawn rect** → clipped across however many cells it spans.

Clipping is the general fallback; it is only *paid* where geometry crosses a boundary.

### 3.3 Choosing the cell size

Recommend **`cellSize` = the tile layer's grid size (16)**, passed explicitly by the loader rather
than derived.

The tradeoff runs in both directions and neither extreme is right:

- **Too fine (e.g. 8)** — a full 16 px tile fragments into 4 cells, so `QueryArea` returns 4 candidates
  where 1 would do, and the fragments reintroduce interior edges (§7).
- **Too coarse (e.g. 64)** — each cell holds up to 16 tiles' worth of boxes, and *every distinct
  arrangement of those 16 tiles is a distinct palette entry*. Palette size grows combinatorially and
  dedup stops paying for itself.

Matching the coarsest source keeps finer sources nesting for free and never fragments them.

**Phase-1 normalization worth having:** in `Bake`, if a cell's fragments fully cover the cell, replace
them with a single cell-sized box. That one special case handles the overwhelmingly common
"solid block" — including the 4-quadrant IntGrid case — and costs nothing. General intra-cell merging
is phase 2.

### 3.4 Tile identity and mutation *(answer to Q4 — building it in now)*

Three pieces, all cheap to add up front and awkward to retrofit:

1. **`CollisionLayer::SetCell(row, col, shapeIndex)`** — swap a cell's shape at runtime. Destructible
   terrain becomes `SetCell(r, c, EMPTY_SHAPE)`; a door becomes a toggle between two palette indices.
   The palette is append-only after bake, so a "door open" shape must exist at bake time or be added
   via a `uint16_t AddShape(TileShape)`.

2. **`ColliderRef::tileIndex`** — so a collision can report *which* cell was hit (§3.5).

3. **`TileShape::flags`** — declared now, unused. One-way platforms, triggers, and surface materials
   all live here, and adding a field to a baked struct later means re-touching the builder, the
   palette dedup (flags participate in equality), and every call site.

> **Interaction to remember:** `SetCell` and phase-2 greedy merging conflict — mutating a cell must
> invalidate any merged rect covering it. Simplest correct answer when merging lands: merged rects are
> a separate array, and `SetCell` marks the affected row-run dirty for rebuild. Cheap because runs are
> short and mutation is rare.

### 3.5 The shared candidate type

```cpp
namespace sc::physics {
    struct ColliderRef {
        sf::FloatRect box;                                    // world space, already resolved
        entt::entity  entity    = entt::null;                 // null ⇒ this is a tile
        uint32_t      tileIndex = world::CollisionLayer::INVALID_CELL;  // valid when entity == null
    };

    using CandidateBuffer = std::vector<ColliderRef>;
}
```

24 bytes, trivially copyable, contiguous. `entity` keeps collision *events* possible later (damage on
touch, "what did I land on"); `tileIndex` is the same affordance for tiles and feeds straight back
into `SetCell`.

`math::Hitbox` is already relocated to `engine/include/engine/math/Hitbox.h`, so
`world::CollisionLayer` has no ECS dependency. ✔

### 3.6 What `TileColliderCache` becomes

It already computes exactly "hitboxes for (tileset, tileId)". It stays, but purely as **load-time
machinery for the baker** — the baker asks it for a tile's hitboxes and feeds them to `AddBox`. It is
no longer consulted at runtime, and `SpawnTileCollider` disappears entirely.

---

## 4. Where `CollisionLayer` lives *(answer to Q2 — decided)*

`CollisionLayer` **drops `ILevelLayer`** and `Level` owns exactly one:

```cpp
class Level {
    // ...
    const CollisionLayer& GetCollisionLayer() const { return m_collisionLayer; }
    CollisionLayer&       GetCollisionLayer()       { return m_collisionLayer; }
private:
    math::SpatialGrid m_spatialGrid;     // entities
    CollisionLayer    m_collisionLayer;  // static tile geometry
    std::vector<std::unique_ptr<ILevelLayer>> m_layers;   // render order only
};
```

Rationale: the layer stack is documented as *"expose layer stack for renderer"*, and the collision
grid is a **baked product of several source layers**, not any one of them — it has no place in a
render-ordering list. The dead `LayerType::Collision` branch is already removed from `render_system`. ✔

`LayerType::Collision` can stay in the enum for a future hand-painted *visual* debug layer, or be
deleted. Nothing depends on it now.

Multiple collision sources are handled by the **builder merging them into one grid** (§3.2), not by a
vector of layers — which is what makes "16 px tiles + 8 px IntGrid" a non-problem.

Since `CollisionLayer` is a `Level` member and `Level`'s size is known at construction, the grid can
be sized in the constructor the same way `m_spatialGrid` already is, and `Bake()`'s result move-assigned
in once the loader has run.

---

## 5. Authoring sources *(answer to Q1)*

**Primary — tileset tile custom data.** Already in use, already parsed by `HitboxUtils`. Collision
follows the art: paint a wall tile anywhere and it's solid, with zero extra authoring.

**Secondary — the 8 px IntGrid layer**, now present in `world_test1.ldtk` as `"IntGrid"` with a single
value `1`. `LDtkLoader` reads it via `LayerType::IntGrid` and `Layer::getIntGridVal(x, y)`. This is for
precision work the 16 px art grid can't express: a doorway notch, a ledge lip, an invisible wall.

Both become one-screen functions over the same primitive:

```cpp
void BakeTileLayer  (const ldtk::Layer&, TileColliderCache&, CollisionLayerBuilder&);
void BakeIntGridLayer(const ldtk::Layer&, CollisionLayerBuilder&);
```

**These are not `ILayerBuilder`s.** They don't produce an `ILevelLayer`, and making them return
`nullptr` to signal "skip" would abuse `LoadLevel`'s existing skip path. Instead `LoadLevel` gets an
explicit collision pre-pass before the layer loop:

```cpp
CollisionLayerBuilder collision(origin, tileSize, gridSize);
for (const auto& ldtkLayer : ldtkLevel.allLayers()) {
    if (ldtkLayer.getType() == ldtk::LayerType::IntGrid)  BakeIntGridLayer(ldtkLayer, collision);
    else if (ldtkLayer.hasTileset())                      BakeTileLayer(ldtkLayer, ctx.tileColliders, collision);
}
engineLevel->SetCollisionLayer(collision.Bake());

// ...then the existing render-layer loop, unchanged
```

Ordering-independent, and adding a third source later is one more `if`.

Two notes on the IntGrid:

- **`isVisible()` must not gate it.** The existing layer loop skips invisible layers; a collision
  IntGrid is very likely to be hidden while editing. The pre-pass has to ignore visibility.
- **Sources are additive.** An IntGrid value cannot *subtract* solidity from an art tile. If you later
  want "decorative tile that shouldn't block", that needs an explicit clear-value convention — e.g.
  reserve IntGrid value `2` as "carve", processed after all additive sources. Cheap to add later
  *provided* `AddBox` isn't the only primitive; worth keeping in mind but not building now.

---

## 6. Physics system refactor

### 6.1 Signature

`UpdateKinematics` needs both broad phases. `Level` owns both, and `render_system` already takes a
`world::Level&`, so the dependency direction is established:

```cpp
// before
void UpdateKinematics(Registry&, math::SpatialGrid& grid, float timeStep);

// after
void UpdateKinematics(Registry&, world::Level& level, float timeStep);
```

`RegisterEntityCollisions` and `BuildSpatialGrid` keep taking the `SpatialGrid` directly — they only
concern entities. `BuildSpatialGrid` gets *cheaper by 5000 entities*, and the double-indexing flagged
in the last report disappears, because nothing but the factory inserts anything anymore.

> If a `physics::CollisionWorld` bundling `{SpatialGrid, CollisionLayer}` is ever wanted (the Box2D
> `b2World` / Godot `PhysicsServer2D` shape), it's a rename of what `Level` already holds. Not worth
> introducing now.

### 6.2 Query once per step, not once per slide

`MoveAndResolve` currently re-queries the broad phase inside its 3-iteration slide loop. **That is
provably unnecessary.** After a slide, the new position is `pos₀ + delta₀·t` and the remaining delta
is `delta₀·(1−t)` with components zeroed, so the new query bounds are a strict subset of the original
swept bounds:

```
bounds(pos₀ + delta₀·t) ⊆ sweptBounds(pos₀, delta₀)          for t ∈ [0,1]
(pos₀ + delta₀·t) + delta₀·(1−t) = pos₀ + delta₀             the far extent is unchanged
```

Hoisting the query above the loop is a **3× cut in broad-phase work** with no behavioural change.

### 6.3 Narrow phase

```cpp
math::SweepResult FindClosestCollision(
    const TransformComponent& trans,
    const BoxColliderComponent& col,
    sf::Vector2f delta,
    const physics::CandidateBuffer& candidates,   // no registry, no per-candidate component fetch
    entt::entity self);
```

The `reg.all_of` / `reg.get` calls vanish. `self` is still needed to skip the mover's own entry from
the entity grid; tiles can never be `self`.

### 6.4 Resulting frame shape

```cpp
for each (entity, trans, vel, col) in dynamicView:
    swept = ExpandBounds(GetEntityBounds(trans, col), delta)

    candidates.clear();                                   // capacity retained
    level.GetCollisionLayer().QueryArea(swept, candidates);
    level.GetSpatialGrid().Query(swept, candidates, reg);  // appends resolved rects

    for slide in 0..MAX_SLIDES:                           // reuses candidates
        sweep = FindClosestCollision(...)
        ...resolve...

    if moved: grid.MoveEntity(entity, GetEntityBounds(trans, col));
```

One allocation-free buffer for the whole frame.

---

## 7. Flat-wall snagging, and why baking helps

A known swept-AABB failure: a wall of 16 px tiles presents *interior* edges that don't exist in the
real surface. Sliding along it, floating-point error puts the mover a fraction of a pixel into the
next tile and generates a spurious perpendicular normal — the character catches on seams.

Two mitigations, in order of value:

**Greedy merging at bake time** *(the standard fix — Unity's "Used By Composite", Godot's and Tiled's
collision merge).* Runs of horizontally adjacent full-cell shapes merge into one wide rect. A 40-tile
floor becomes 1 candidate instead of 40, removing the interior edges **and** cutting candidate count by
an order of magnitude on the most common query.

Implementation that fits the grid model: keep `m_cells` for lookup, add `m_mergedRects` plus a parallel
per-cell `m_rectIndex`. `QueryArea` emits a merged rect for full-solid cells (deduping by rect index
within the query — cheap, the query touches few cells) and per-cell hitboxes for partial shapes. See
§3.4 for the `SetCell` invalidation interaction.

**Deterministic iteration order.** Tie-breaking in `FindClosestCollision` is `sweep.time < closest.time`
— strictly less-than, so on a flat wall where every candidate hits at the same `t`, the *first* one's
normal wins. Today "first" means arbitrary registry order. Row-major cell iteration at least makes it
reproducible, turning an intermittent bug into a debuggable one.

Merging is **phase 2** — it needs care around partial shapes and one-way tiles, and phase 1 should be
verified working first. The §3.3 "cell fully covered ⇒ one box" normalization is the phase-1 down
payment on it.

---

## 8. Phasing

**Phase 1 — tiles leave the ECS.**
1. ~~Move `Hitbox` to `engine/math/`~~ ✔ done
2. Add `physics::ColliderRef` / `CandidateBuffer` (`engine/physics/ColliderRef.h`).
3. Rewrite `CollisionLayer`: palette + index grid + `QueryArea` + `Get/SetCell` + `TileShape::flags`.
   Drop `ILevelLayer`, delete `IsSolid`/`SetData`.
4. `CollisionLayerBuilder` with `AddBox` + `Bake` (engine side — it's generic geometry, not LDtk).
5. `Level` owns a `CollisionLayer`; add `GetCollisionLayer()` / `SetCollisionLayer()`.
6. Game-side `BakeTileLayer` + `BakeIntGridLayer`; collision pre-pass in `LoadLevel`.
7. `SpatialGrid::Query` overload appending resolved `ColliderRef`s.
8. `UpdateKinematics` takes `Level&`, queries both, flat narrow phase.
9. Delete `SpawnTileCollider`; `TileColliderCache` becomes baker-only.
10. **`CollisionOverlay` reads tile shapes from the `CollisionLayer`.** Terrain stops being entities, so
    the current overlay would render *nothing* for it — this must land with phase 1, not after.

**Phase 2 — performance and correctness.**
11. Hoist the broad-phase query out of the slide loop (§6.2).
12. Greedy row merging at bake time + `SetCell` invalidation (§7, §3.4).

**Phase 3 — capability.**
13. Collision layers/masks (`layer`/`mask` bitmask on `BoxColliderComponent`, checked in narrow phase).
14. One-way platforms & triggers, using `TileShape::flags` and `ColliderRef::tileIndex`.
15. Subtractive IntGrid "carve" values (§5), if authoring ever needs it.
16. Replace `SpatialGrid::Query`'s `sort`+`unique` with a per-query stamp buffer.

Phase 1 is a coherent commit on its own. Nothing in phases 2–3 is required to make it correct.

---

## 9. Remaining unknowns

Nothing blocking — these get answered by writing the code, not by deciding up front.

- **IntGrid value → shape mapping.** Today there is one value (`1` = solid). `BakeIntGridLayer` can
  hardcode "any non-zero value ⇒ full cell" for phase 1. When a second value appears (one-way, water,
  ice), it needs a `value → TileShape` table, which is naturally game-side config.
- **The IntGrid layer's origin.** `Layer::getOffset()` exists and LDtk layers can be offset; the
  builder takes an explicit `origin` for this reason. Worth asserting it's zero in phase 1 rather than
  silently mishandling it.
- **Palette size in practice.** If it ever exceeds a few dozen entries for a normal level, the cell
  size is too coarse (§3.3). Cheap to log after `Bake()` and worth watching once.
