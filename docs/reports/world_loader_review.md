# Review: `very_cool_rpg/src/loaders/world_loader.{h,cpp}`

Date: 2026-08-03
Scope: read-only review, no code changed.

## TL;DR

The instinct is right — this file is doing too much, and the abstraction
boundary between "parsing LDtk" and "constructing engine objects" doesn't
exist yet. It reads like the first version that got the demo working, not
a settled design. Below is what's actually wrong, roughly ordered by impact.

## 1. One function, five responsibilities

`Load()` → `LoadLevel()` → `ProcessGenericTileLayer()` / `ProcessEntityLayer()`
→ `ProcessCustomData()` / `AddBoxCollider()` currently mixes together:

- **File I/O / existence checks** (`fs::exists`, `ldtkProject.loadFromFile`)
- **LDtk → engine data translation** (layer/tile/entity iteration)
- **Asset loading** (`TileSet` construction from disk paths)
- **ECS mutation** (`registry.CreateEntity`, `AddComponent`, `entityFactory.Spawn`)
- **Ad-hoc JSON parsing of a side-channel** (LDtk "custom data" strings holding
  hitbox JSON, parsed with a raw `nlohmann::json::parse` + hand-rolled schema
  assumptions)
- **Logging/diagnostics** sprinkled at every level

Nothing here is reusable or testable in isolation — you cannot unit-test
"does this LDtk entity produce the right hitboxes" without a live `Registry`,
a real `TileSet` (which touches disk), and a real `EntityFactory`. That's
the concrete cost of the missing boundaries, not just a style complaint.

## 2. It silently does two different jobs and calls them both "collision"

There's a real `CollisionLayer` class in the engine
(`engine/include/engine/world/CollisionLayer.h`) with a tile-grid
`IsSolid()` query clearly designed for the physics system. `world_loader`
**never constructs one**. Instead, `ProcessCustomData()` reads per-tile
custom JSON off the tileset and spawns a *new ECS entity per solid tile*
(`Tile_Collider_x_y`) with a `BoxColliderComponent`.

So today there are two competing collision representations in the engine
(grid-based `CollisionLayer`, and per-entity `BoxColliderComponent` spam
from the loader) and the loader only feeds the second one. Either
`CollisionLayer` is dead code, or the loader is bypassing the design it was
supposed to populate — worth resolving explicitly rather than leaving both
paths live.

Similarly, `ProcessEntityLayer` has a stale comment `// process collisions`
above a loop that does nothing collision-related anymore (it spawns entities
and separately gives *entities* — not tiles — hitboxes). That's a sign this
code has been reshuffled at least once without cleanup.

## 3. Duplicated "parse hitbox JSON into a BoxColliderComponent" logic

`ProcessCustomData` (tile custom-data path) and `ProcessEntityLayer` (entity
custom-data path) both end up calling `AddBoxCollider`, but:

- One catches `json::parse_error` and logs (`ProcessCustomData`).
- The other calls `json::parse(customData)` **unguarded** inside
  `ProcessEntityLayer` (line ~134) — a malformed custom-data string on an
  *entity* tileset will throw and, as far as this file is concerned,
  propagate out of `Load()` uncaught. The tile path and the entity path
  should have identical error-handling but don't.

This is a correctness bug hiding behind duplicated responsibility, not just
a style issue — it's the kind of thing that happens naturally when the same
concept is implemented twice instead of factored into one place.

## 4. Naming vs. behavior mismatch (`GetTileIdFromEntity`)

`GetTileIdFromEntity` computes a *source tileset* tile index for the icon an
LDtk entity happens to be drawn with, purely so the loader can look up
*custom data attached to that tile* to steal hitbox JSON from it. That's a
reasonable trick, but nothing in the name or the surrounding code says
"we're reusing the tile-collision authoring system for entities too." A
future reader (including future-you) will read `ProcessEntityLayer` and
wonder why it's computing tileset math for what should be a straightforward
entity spawn.

## 5. `EntityLayer` construction is a no-op / misleading

`ProcessEntityLayer` builds and returns a `world::EntityLayer`, and
`LoadLevel` adds it to the level — but the function never adds anything to
that layer (`EntityLayer` per its own header comment is just a marker;
entities live only in the ECS `Registry`, keyed by nothing that ties them
back to this layer or level). That's fine as a *design* (renderer queries
ECS directly), but the loader code reads as if `entityLayer` is meaningfully
populated when it's actually just a tag object. Worth at least a comment at
the call site, ideally a constructor that makes the "this is just a marker"
intent unmissable (e.g. `world::EntityLayer::MakeMarker(layerId)`).

## 6. Mixed abstraction levels / control flow smell in `LoadLevel`

The branch in `LoadLevel` that decides Tile vs YSortedTile vs Entity layer
is doing string matching (`layerName.find(YSORTED_LAYER_TAG)`) combined with
enum checks combined with a template parameter selection
(`ProcessGenericTileLayer<world::YSortedTileLayer>`). This is a classic case
for a small factory/strategy: something like
`ILayerProcessor::CanHandle(ldtkLayer) / Process(ldtkLayer, ctx)`, letting
you register new layer kinds (e.g. a future proper `CollisionLayer` builder)
without editing this if/else chain. Right now adding a new layer type means
touching this function's control flow directly.

## 7. Parameter/context sprawl

Nearly every private helper takes some subset of
`{ecs::Registry&, factories::EntityFactory&, fs::path projectDir}` and passes
them along manually. This is the textbook signal for a small `LoadContext`
struct (registry, entityFactory, projectDir, maybe a texture/tileset cache)
threaded through instead of ad-hoc parameter lists — it would also make the
eventual split into multiple files/classes much less noisy.

## 8. Inconsistent style/formatting

Minor, but noticeable: the file mixes tabs and spaces (compare the indentation
inside `Load()` at the bottom, which uses spaces, against everything else,
which uses tabs). Not a functional bug, but it suggests this function was
edited separately from the rest and is a good marker for "this part changed
most recently / least reviewed."

## Suggested direction (not a mandate)

Given the responsibilities identified above, a natural split is:

- `LdtkTileSetLoader` — disk path → `gfx::TileSet` (with caching, since right
  now every layer reloads its tileset from disk with no cache).
- `TileLayerBuilder` — LDtk layer → `TileLayer`/`YSortedTileLayer`, no ECS
  knowledge.
- `EntityLayerBuilder` — LDtk layer → ECS entities via `EntityFactory`, no
  tile/tileset knowledge.
- `HitboxParser` — the one place `json` hitbox arrays get parsed into
  `BoxColliderComponent` data, used identically by both the tile-collider
  and entity-collider paths, with one consistent error-handling policy.
- `WorldLoader` (thin) — orchestrates the above, owns the `LoadContext`.

This isn't a rewrite mandate — it's the shape that falls out once you name
the responsibilities currently fused together in one file. Happy to help
implement it when you're ready; this pass was analysis only, per your
request.
