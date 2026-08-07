# Entity loading & spawning pipeline refactor

**Date:** 2026-08-07
**Scope:** `very_cool_rpg` — entity definitions, blueprints, `EntityFactory`, world loader bridge

---

## 1. What was wrong

The old pipeline built an entity in two places that didn't know about each other.

```
world_loader finds ldtk::Entity
  └─ EntityFactory::Spawn(ldtkEntity, ...)
       ├─ IBlueprint::Build(entity, ldtkData, textureCache, projDir)   ← transform + sprite + game components
       └─ RegisterEntityCollisions(grid, entity)                       ← indexes into the spatial grid
  └─ ...back in EntityLayerBuilder...
       └─ ApplyCustomData(entity, ...)                                 ← BoxColliderComponent added HERE
```

Three concrete problems:

1. **`Spawn` required LDtk data.** The signature took an `ldtk::Entity`, a texture cache and a project
   directory, so nothing outside the world loader could ever create an entity. There was no way to
   spawn a dragon at runtime.

2. **`RegisterEntityCollisions` ran before the collider existed.** It was called at the end of `Spawn`,
   but the `BoxColliderComponent` wasn't attached until `ApplyCustomData` ran afterwards in
   `EntityLayerBuilder`. So it hit its own "no collider" guard and no-oped for **every entity, on every
   load**. The bug was invisible only because `LevelLoader` ran a full `BuildSpatialGrid` sweep after
   all layers were built, which quietly re-did the work.

3. **Per-type data was re-derived per instance.** Every single placed entity re-did the tileset lookup,
   the tile-id reverse lookup, the custom-data JSON parse and the pivot maths — all of which depend
   only on the entity *type*, not the instance. Same for tiles: the same tile id repeated hundreds of
   times per level, re-parsing the same JSON string each time.

The root cause behind all three is the same: **there was no representation of an entity type**. The
LDtk file *was* the type description, so anything that needed type information needed the file open.

---

## 2. The new pipeline

```
startup     blueprints::RegisterAll(factory)          EntityType -> IEntityBlueprint
load, once  LoadEntityDefinitions(project, factory)   ldtk::EntityDef -> EntityDefinition
per level   EntityLayerBuilder                        ldtk::Entity -> SpawnParams -> Spawn()
runtime     factory.Spawn(type, params)               no LDtk involved at all
```

The split is **per-type data (`EntityDefinition`) vs per-instance data (`SpawnParams`)**. LDtk is now
read exactly once per type, up front, into a struct containing no LDtk types.

`EntityDefinitionLoader` and `EntityLayerBuilder` are the only two files left that see both LDtk types
and spawning types.

### 2.1 `entities/EntityType.h`

An enum whose values *are* the FNV-1a hash of the LDtk identifier, so parsing an identifier is a
constexpr hash and a cast — no switch, no lookup table to keep in sync:

```cpp
constexpr uint64_t hash(std::string_view str);       // FNV-1a, 64-bit

enum class EntityType : uint64_t {
    Player = hash("Player"),
    Dragon = hash("Dragon"),
};

constexpr EntityType FromLdtkIdentifier(std::string_view identifier) {
    return static_cast<EntityType>(hash(identifier));
}
```

The char is cast through `uint8_t` before widening — `char` is signed on MSVC, so a non-ASCII byte
would otherwise sign-extend and produce a different hash than the same string on a platform where
`char` is unsigned.

### 2.2 `entities/EntityDefinition.h` — per type

`type`, `debugName`, `shared_ptr<Texture2D> texture`, `textureRect`, `pivot`, and
`vector<Hitbox> hitboxes` **already pivot-adjusted**. No LDtk types, no `fs::path`, no texture cache.

### 2.3 `entities/SpawnParams.h` — per instance

`position` and `layerUid`, plus a marked slot for future per-instance collider overrides.

### 2.4 `EntityFactory` — the fix that matters

`Spawn` now performs the common assembly itself, then calls the blueprint, then indexes:

```cpp
entity.AddComponent<TransformComponent>(params.position, definition.pivot);
if (definition.texture)          entity.AddComponent<SpriteComponent>(...);
if (!definition.hitboxes.empty()) entity.AddComponent<BoxColliderComponent>().hitboxes = definition.hitboxes;

blueprintIt->second->Build(entity, params);   // type specific components only

if (m_currentLevel)
    physics_system::RegisterEntityCollisions(m_currentLevel->GetSpatialGrid(), entity);
```

This is why problem 2 is **eliminated rather than fixed**: a blueprint cannot forget the collider or
attach it late, because a blueprint never touches the collider at all. There is exactly one point in
the codebase where an entity is structurally complete, and the grid registration sits at that point.

### 2.5 Blueprints

`IBlueprint` → `IEntityBlueprint`, and the interface shrank from

```cpp
virtual void Build(Entity&, const ldtk::Entity&, TextureCache&, const fs::path&) const = 0;
```

to

```cpp
virtual void Build(Entity& entity, const entities::SpawnParams& params) const = 0;
```

`PlayerBlueprint::Build` is now three `AddComponent` calls. Both blueprint classes lost their
constructors and members entirely — they are stateless now.

### 2.6 `TileColliderCache`

Tile hitboxes keyed by `(tilesetUid << 32) | tileId`. **Misses are cached too**, as an empty vector —
most tiles in a tileset have no custom data, and without caching the miss every one of them re-parses
on every instance.

Explicitly a stopgap: one ECS entity per solid tile is the wrong shape. Tiles aren't entities, and
`world::CollisionLayer` is where static level geometry belongs. Both `TileColliderCache` and
`SpawnTileCollider` are deleted when that lands.

---

## 3. Files

**Added**

| File | Purpose |
|---|---|
| `src/entities/EntityType.h` | hash-valued enum + `FromLdtkIdentifier` |
| `src/entities/EntityDefinition.h` | per-type data, LDtk-free |
| `src/entities/SpawnParams.h` | per-instance data |
| `src/blueprints/IEntityBlueprint.h` | new blueprint interface |
| `src/loaders/world/EntityDefinitionLoader.h/.cpp` | `ldtk::EntityDef` -> `EntityDefinition`, once |
| `src/loaders/world/TileColliderCache.h/.cpp` | per-tile hitbox cache |

**Deleted**

| File | Why |
|---|---|
| `src/blueprints/IBlueprint.h` | replaced by `IEntityBlueprint.h` |
| `src/loaders/world/CustomData.h/.cpp` | field-handler registry; its job moved to definition-load time |
| `src/utils/assemblers/LdtkAssemblers.h/.cpp` | `AttachTransform`/`AttachSprite` now live inside `Spawn` |

**Modified:** `EntityFactory.h/.cpp`, `BlueprintRegistry.h/.cpp`, `PlayerBlueprint.h/.cpp`,
`DragonBlueprint.h/.cpp`, `blueprints/includes.h`, `HitboxUtils.h/.cpp`, `EntityLayerBuilder.cpp`,
`TileLayerBuilder.cpp`, `LevelLoader.cpp`, `LoadContext.h`, `world_loader.cpp`,
`GameplayLayer.h/.cpp`.

---

## 4. Known remaining issues

Ordered by how likely they are to bite.

### 4.1 An entity type with no tileset gets no definition at all

`EntityDefinitionLoader.cpp:47` — when `ldtkDef.tileset` is null the loop `continue`s **before**
`factory.SetDefinition(...)`. The definition is never stored, so `Spawn` rejects the type outright with
*"no definition loaded"*. Invisible types (triggers, spawn points, camera anchors) that legitimately
want only a transform can never be spawned.

Fix: store the definition first, then skip only the texture/hitbox work:

```cpp
if (!ldtkDef.tileset) {
    LOG_W("entity definition '%s' has no tileset, so it gets no sprite or hitboxes", name.c_str());
    factory.SetDefinition(type, std::move(definition));
    continue;
}
```

### 4.2 A mismatched enumerator fails with an unreadable message

`Register` no longer takes the identifier string, so nothing verifies that `EntityType::Player ==
hash("Player")`. A typo in either place means the definition is keyed by the file's hash and the
blueprint by the enumerator's, and `Spawn` logs
`no blueprint registered for entity type 11831194018420276491`. It fails loudly, but that number
doesn't tell you which entity is broken. Cheap improvement: when the definition *is* found but the
blueprint isn't, log `definitionIt->second.debugName` instead of the raw hash.

### 4.3 Stale comment in `BlueprintRegistry.cpp`

Lines 11–12 still say *"the string must match the LDtk entity identifier exactly ... Register verifies
the pair rather than trusting it"*. `Register` takes no string and verifies nothing. The comment
should describe 4.2's actual behaviour instead.

### 4.4 `LoadContext::currentLevel` is write-only

`LevelLoader.cpp:15` sets it, nothing ever reads it — the factory's own `m_currentLevel` is what
actually gets used. Delete the field and the assignment, or a future builder will read it thinking
it's load-bearing.

### 4.5 `GetTileIdFromRect` ignores spacing and padding in the column count

`HitboxUtils.cpp:56`:

```cpp
const int columns = tileset.texture_size.x / tileset.tile_size;   // wrong when spacing/padding != 0
```

The stride is correctly `tile_size + spacing`, but the column count uses the bare `tile_size`. For the
current tilesets (`spacing == padding == 0`) both agree, so nothing is broken today. With any spacing
this silently returns the wrong tile id and entities get the wrong hitboxes. Correct form:

```cpp
const int stride  = tileset.tile_size + tileset.spacing;
const int columns = (tileset.texture_size.x - tileset.padding * 2 + tileset.spacing) / stride;
```

(LDtkLoader is vendored as headers + a prebuilt lib, so `getTileTexturePos`'s exact implementation
couldn't be read to confirm — this is the standard LDtk grid layout. `Tileset` exposes no inverse
helper, so the reverse lookup has to stay hand-rolled either way. It is at least now done once per
type instead of once per instance.)

### 4.6 `LevelLoader` still runs a full `BuildSpatialGrid` sweep

Factory-spawned entities now index themselves correctly, but tile colliders are created directly by
`SpawnTileCollider` and never touch the factory, so the sweep still has real work to do. Entities end
up visited twice; harmless, because `InsertEntity` ignores anything already present. Goes away with
`CollisionLayer`.

### 4.7 `Register`'s null-blueprint log passes an enum through varargs

`EntityFactory.cpp:15` — `LOG_W("... '%llu'", type)` passes `EntityType` (not `uint64_t`) to a
printf-style variadic. It works on MSVC because the underlying type is already `uint64_t`, but the
other two call sites cast explicitly and this one should match.

### 4.8 Unrelated files staged for commit

`test.cpp` and `last_session.txt` are staged. Probably not intended for this commit.

---

## 5. Deferred by design

- **`world::CollisionLayer`** — static tile geometry out of the ECS. Removes `TileColliderCache`,
  `SpawnTileCollider`, and the full grid sweep in 4.6.
- **`Level::FindLayer(name)`** — `GameplayLayer::m_entityLayerUid` currently grabs the *first*
  `LayerType::Entity` layer. Fine with one; ambiguous with several.
- **Per-instance collider overrides** — slot reserved in `SpawnParams`.
- **Custom-field extensibility** — `CustomData`'s handler registry is gone. Its natural replacement is
  reading authored fields into `EntityDefinition` members at definition-load time, so per-instance
  spawning stays free of JSON.

---

## 6. Verification

Not built — per `CLAUDE.md` I don't run MSVC builds. Verified by inspection: no remaining references to
`IBlueprint`, `ApplyCustomData`, `AddBoxCollider`, `GetTileIdFromEntity`, `CalcPivotOffset`,
`GetTextureCache`, `RegisterBuiltinCustomDataFields`, `LdtkAssemblers`, `AttachTransform` or
`AttachSprite`, and no `#include` of any deleted header.

The strongest runtime check is the debug overlay: **F1 → Collisions → spatial grid**. Every dragon and
rock should draw arrows to its cells. Then, because `RegisterEntityCollisions` actually fires now, a
runtime

```cpp
m_entityFactory.Spawn(entities::EntityType::Dragon, { .position = pos, .layerUid = m_entityLayerUid });
```

should show its arrows immediately — under the old pipeline it would have shown none.
