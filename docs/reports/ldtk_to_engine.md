# From LDtk to the engine

Date: 2026-10-11. Commit: `be1548a`.

How an LDtk project is structured, how `very_cool_rpg` turns one into engine objects, which rules your LDtk project has to follow for that to work, and what is ignored today. Written from the loader code in `very_cool_rpg/src/loaders/`, the vendored LDtkLoader headers (1.5.3.1), and `assets/world_test1.ldtk`.

Two things could not be checked, because LDtkLoader is vendored as headers and a prebuilt library: whether `ldtk::Tile::getPosition()` includes the layer's pixel offset, and which tile `ldtk::Entity::getTextureRect()` returns when an instance overrides the definition's tile. Both are marked below.

---

## 1. The LDtk hierarchy

```
Project (one .ldtk file)
├── Definitions            shared by everything in the file
│   ├── Layer definitions      name, type, grid size, tileset, auto-rules
│   ├── Entity definitions     name, size, pivot, tags, display tile, field definitions
│   ├── Tilesets               image, grid size, spacing, padding, per-tile custom data
│   ├── Enums
│   └── Level field definitions
└── Worlds                 one implicit world, or several named ones
    └── Levels                 name, uid, size, position in the world, depth, neighbours
        └── Layer instances        one per layer definition, stored top first
            ├── tiles                  (Tiles, AutoLayer and IntGrid-with-rules layers)
            ├── int grid values        (IntGrid layers)
            └── entity instances       (Entities layers)
                └── field instances
```

Points that matter for the loader:

- **Definitions are per project, not per level.** Every level has an instance of every layer definition. A layer's uid in the loader is the uid of its **definition**, so it is the same number in every level.
- **Four layer types.** `Tiles` (painted by hand), `IntGrid` (integer cells, optionally with auto-rules that produce tiles), `AutoLayer` (rules only, reading another IntGrid layer), and `Entities`.
- **A tile** has a pixel position, a source rectangle in the tileset, flip bits and an alpha.
- **An entity instance** has a permanent id (`iid`), a pixel position, a size, and values for the fields its definition declares. Its position is the position of its **pivot**, not of its top-left corner.
- **A level** has a position in its world (`worldX`, `worldY`, `worldDepth`) and a list of neighbours. How levels are arranged is the world's **layout**: Free, GridVania, LinearHorizontal or LinearVertical.
- **Ids.** Definitions and levels have an integer `uid`, unique in the project. Levels, layers, entities and worlds also have an `iid` string, unique everywhere.

### Worlds, and what "another LDtk world" can mean

A project has one world by default. Its levels are stored at the root of the file and the world has no name. The project setting "multi-worlds" changes that: the file then has a `worlds` array, each world has a name, its own layout and its own levels.

All worlds in one file share the same definitions, and level uids stay unique across them. An entity reference field already stores the target's world, level, layer and entity ids, so a reference can point into another world of the same file.

A second `.ldtk` **file** is a different thing: separate definitions, unrelated uids, and no way to reference across.

| | Multi-worlds in one file | One file per world |
|---|---|---|
| Layer and entity definitions | shared | duplicated and kept in sync by hand |
| Level uids | unique across worlds | can collide |
| Entity reference from one world to another | works in LDtk | not possible |
| Loader change needed | iterate `allWorlds()` | a second load, uid remapping, and a custom way to express a destination |

For "a secret room that teleports you to another world", multi-worlds in one file is the option to choose.

---

## 2. The engine side

```
sc::world::World              a map of level uid to Level
└── sc::world::Level          name, uid, size in pixels
    ├── layers, bottom to top     TileLayer, YSortedTileLayer, EntityLayer
    ├── CollisionLayer            static tile collision, one grid per level
    └── SpatialGrid               which entities are in which cell

sc::ecs::Registry             every entity of every level
                              each one carries a LevelComponent with its level's uid

game::factories::EntityFactory
├── EntityDefinition per type     texture, rect, pivot, hitboxes
└── blueprint per type            the type-specific components
```

An `EntityLayer` holds no entities. It marks the place in the layer stack where the entities are drawn.

---

## 3. What maps to what

| LDtk | Engine or game | Kept | Dropped |
|---|---|---|---|
| Project | nothing | The `ldtk::Project` only lives during `world_loader::Load`. | |
| World | `sc::world::World` | its levels | name, layout. Only the unnamed world is read. |
| Level | `sc::world::Level` | name, uid, pixel size | world position, depth, neighbours, level fields, background |
| Layer definition | `LayerId { name, uid }` on each layer | name and definition uid | everything else |
| Tiles, IntGrid or AutoLayer layer with a tileset | `TileLayer`, or `YSortedTileLayer` if the name has the `YS` tag | its tiles | opacity, IntGrid values |
| Entities layer | `EntityLayer` marker, plus entities in the registry | | |
| Tile | `TileInstance { pixelPos, textureRect }` | position, source rectangle | flips (a warning is logged), alpha |
| Tileset | `sc::graphics::TileSet`, cached by file path | the image | spacing and padding (LDtk's own rectangles are used, so they are honoured for drawing) |
| Tile custom data | hitboxes | the `hitboxes` array | any other key |
| Entity definition | `entities::EntityDefinition` | name (as a hash and as a debug name), display tile rectangle, pivot, hitboxes of the display tile | size, resizable flags, tags, colour |
| Entity instance | one ECS entity | position, layer definition uid, fields | `iid`, size, per-instance tile, tags (apart from `marker`) |
| Field instance | an entry in `SpawnParams::fields` | Int, Float, Bool, String, EntityRef | Point, Enum, Color, FilePath and all array types (a warning is logged) |
| EntityRef field | `EntityRefValue { levelUid, pos, entityType }` | the target's level uid and position | the target's `iid` and world |

---

## 4. The load, step by step

### 4.1 Start

`GameplayLayer::OnAttach` (`very_cool_rpg/src/layers/GameplayLayer.cpp:57-64`) registers the blueprints and calls `world_loader::Load` with the path to `world_test1.ldtk`.

### 4.2 `world_loader::Load` (`loaders/world_loader.cpp:17-56`)

1. Checks that the file exists and parses it with LDtkLoader. A parse failure is caught and logged.
2. Gives the factory the registry.
3. Calls `LoadEntityDefinitions` once for the whole project.
4. Creates a `TileColliderCache`, a `LoadContext` and the list of layer builders.
5. For each level of the unnamed world, calls `LoadLevel` and adds the result to the engine `World`.

### 4.3 Entity definitions (`loaders/world/EntityDefinitionLoader.cpp:36-78`)

For each LDtk entity definition:

1. The type is the FNV-1a hash of the identifier. This is why `EntityType::Chest` must equal `hash("Chest")`.
2. Pivot and display rectangle are copied.
3. If the definition has no tileset, it stops here and **no definition is stored**, so the type cannot be spawned.
4. The texture is loaded, through a cache that is local to this function.
5. The tile id is computed back from the display rectangle (`HitboxUtils.cpp:55-61`), and that tile's custom data is parsed for hitboxes.
6. The hitboxes are shifted by the pivot. In the file they are relative to the tile's top-left corner. An entity's position is its pivot, so the boxes are stored relative to the pivot.

### 4.4 One level (`loaders/world/LevelLoader.cpp:9-32`)

1. Creates the `Level` with its name, uid and pixel size. The spatial grid uses 64-pixel cells.
2. Tells the factory that new entities go into this level.
3. Bakes the collision layer (4.5).
4. Walks the LDtk layers in reverse, because LDtk stores the top layer first and the engine wants bottom first. Invisible layers are skipped. Each visible layer goes to the first builder that accepts it (4.6).

### 4.5 Static collision (`loaders/world/collision_baker.cpp:65-87`)

1. The cell size is the smallest grid size among the level's IntGrid and Tiles layers (`:17-36`).
2. For every IntGrid layer that has a tileset and every Tiles layer, **including invisible ones**, each tile is looked up in the `TileColliderCache`. The cache parses a tile's custom data once and remembers it, including for tiles that have none.
3. Each hitbox is added in level coordinates. The builder cuts it along cell borders.
4. `Build` sorts and deduplicates the boxes of each cell. Cells with identical boxes share one shape.

Pure AutoLayer layers are not baked.

### 4.6 Layer builders (`loaders/world/LayerBuilderRegistry.cpp:17-36`)

Tags are read from the layer name first (5.1). Then the builders are tried in this order:

| Builder | Accepts | Produces |
|---|---|---|
| `YSortedTileLayerBuilder` | has a tileset, is not an Entities layer, has the tag `YS` | `YSortedTileLayer` |
| `TileLayerBuilder` | has a tileset, is not an Entities layer | `TileLayer` |
| `EntityLayerBuilder` | is an Entities layer | `EntityLayer`, and spawns the entities |

A layer no builder accepts (an IntGrid layer with no tileset, for instance) is logged and left out.

**Tile layers** (`TileLayerBuilder.cpp:21-55`): the tileset image is loaded through the `TileSetManager`. Each LDtk tile becomes a `TileInstance` with its pixel position and source rectangle. A `TileLayer` turns them into one vertex array. A `YSortedTileLayer` keeps the list. *Not verified: whether the position includes the layer's pixel offset. All offsets in the current project are zero.*

**Entity layers** (`EntityLayerBuilder.cpp:84-107`): every entity instance that does not have the `marker` tag is spawned with its position, the layer's definition uid, and its fields.

### 4.7 Spawning (`factories/EntityFactory.cpp:26-76`)

1. Looks up the blueprint and the definition for the type. If either is missing, it logs a warning and spawns nothing.
2. Creates the entity, named after the definition.
3. Adds `TransformComponent` (position, pivot) and `LevelComponent`.
4. Adds `SpriteComponent` if the definition has a texture.
5. Adds `BoxColliderComponent` if the definition has hitboxes.
6. Calls the blueprint, which adds the type-specific components and may read `params.fields`.
7. Registers the entity in the level's spatial grid.

### 4.8 After the load

`GameplayLayer` takes the level named `World_Level_0`, makes it current, sets the camera bounds to its size, and finds the entity that has a `PlayerTag`.

All levels stay loaded for the whole run. Only the current one is simulated and drawn.

---

## 5. Rules your LDtk project has to follow

None of these is enforced by LDtk. They are conventions of this loader.

### 5.1 Layer names carry tags

A layer whose name starts with `_` has tags: every word that is followed by another `_` (`LayerTags.cpp:7-21`). `_YS_Decorations` has the tag `YS`. The only tag with a meaning today is `YS`, "sort this layer's tiles by Y together with the entities".

Take care with longer names: `_YS_Trees_Big` has the tags `YS` and `Trees`.

### 5.2 Draw order follows the layer order in LDtk

The top layer in LDtk's list is drawn last. A plain tile layer is a boundary: everything queued below it is sorted and drawn before it. `_YS_` layers and the entity layer between two boundaries are sorted **together**.

In the current project, from bottom to top: `Background` (plain), then `_YS_Decorations`, `_YS_Decorations2` and `Entities`, which sort as one group. If you put a plain tile layer between a `_YS_` layer and `Entities`, they stop sorting against each other.

### 5.3 Entity identifiers are code

The identifier of an entity definition must match an enumerator in `entities/EntityType.h` exactly, and that type needs a registered blueprint. An entity with an unknown identifier is skipped with a warning.

### 5.4 Hitboxes live in tile custom data

The custom data of a tile is a JSON object. The loader reads one key:

```json
{ "hitboxes": [ { "x": 12, "y": 30, "w": 40, "h": 28, "trigger": false } ] }
```

Coordinates are pixels from the tile's top-left corner. `trigger` is optional. `sc_editor` writes this format.

- For a **tile layer**, every tile that has hitboxes contributes them to the level's collision.
- For an **entity**, the hitboxes are those of the tile chosen as the definition's display tile. All instances of the type share them.

A hitbox with `"trigger": true` on a tile that is used in a tile layer currently becomes solid. See 2.3 in the code review.

### 5.5 Markers

An entity definition tagged `marker` in LDtk is never spawned. It exists so other entities can point at it with an entity reference field. `WarpTarget` is the only one. Because a marker is not spawned, it needs no blueprint, no enumerator and no tileset.

### 5.6 Known fields

| Entity | Field | Type | Used for |
|---|---|---|---|
| `TeleportationWell` | `destination` | EntityRef | The level and position the player is sent to. Read in `WellBlueprint.cpp:8-14`. |

A field that is left empty in LDtk is not in the bag, so a blueprint sees it the same way as a field that does not exist.

### 5.7 Invisible layers

A layer hidden in LDtk is not drawn, but its tiles still contribute collision. That gives you collision-only layers.

For an Entities layer, hidden means its entities are not spawned at all.

### 5.8 Coordinates

Everything in the engine is in **level space**: pixels from the top-left corner of the level. The level's position in the LDtk world is not used. A teleport destination is a level uid and a position inside that level.

---

## 6. The current project

`assets/world_test1.ldtk`, LDtk 1.5.3, multi-worlds off, layout LinearHorizontal.

**Layer definitions, top to bottom**

| Name | Type | Grid | Tileset | Becomes |
|---|---|---|---|---|
| `Entities` | Entities | 64 | | `EntityLayer` |
| `_YS_Decorations2` | IntGrid with rules | 64 | `Decorative_64` | `YSortedTileLayer` |
| `_YS_Decorations` | IntGrid with rules | 32 | `Decorative_32` | `YSortedTileLayer` |
| `Background` | IntGrid with rules | 64 | `My_grass` | `TileLayer` |

All three tile layers are IntGrid layers whose tiles come from auto-rules. No tile is painted by hand and none is flipped.

`Decorative_32` and `Decorative_64` are two tilesets over the same image, `12x12_decorative.png`, with different grids.

**Entity definitions**

| Name | Display tile | Tags | Fields | Hitboxes |
|---|---|---|---|---|
| `Player` | 64 by 64 from `3x3_entities.png` | | | one solid |
| `Dragon` | same sheet | | | two triggers, no solid |
| `Chest` | same sheet | | | one trigger |
| `TeleportationWell` | same sheet | | `destination` (EntityRef) | one solid, one trigger |
| `WarpTarget` | none (an ellipse in the editor) | `marker` | | none |

The only tile-layer tile with hitboxes is tile 0 of `Decorative_32`, with two solid boxes.

**Levels**

| Name | uid | Size | Entities |
|---|---|---|---|
| `World_Level_0` | 70 | 768 by 640 | Player, Dragon, TeleportationWell, WarpTarget |
| `World_Level_1` | 191 | 256 by 256 | Dragon, 2 Chests, TeleportationWell, WarpTarget |

Each well points at the `WarpTarget` of the other level.

`assets/world_test2.ldtk` exists and is not loaded by anything.

---

## 7. What it takes to support several worlds

Assuming multi-worlds in one file (section 1).

1. **Read every world.** `world_loader.cpp:48` asks for the world named `""`. With multi-worlds on, that lookup throws and nothing catches it. Loop over `ldtkProject.allWorlds()`.
2. **Keep one engine `World`.** Level uids are unique across LDtk worlds, and the engine `World` is a map from level uid to level. Putting the levels of all LDtk worlds into it means a teleport across worlds needs no new code: the well's `destination` already resolves to a level uid and a position.
3. **Naming.** After step 2, the engine's `World` corresponds to an LDtk *project*, not an LDtk *world*. If you later need to know which LDtk world a level belongs to (for music, a map screen, or a save file), store the world's name on the `Level`.
4. **The player's layer.** A moved entity keeps the uid of the entity layer it was spawned in. That is the layer **definition** uid, which is shared by all worlds of the file, so it keeps working as long as the destination level has that layer visible. See 2.10 in the code review.

Nothing else in the load path depends on there being one world.

---

## 8. Ignored today, and when it will matter

| Ignored | Matters when |
|---|---|
| Level world position and neighbours | You want to walk from one level into the next. This is how a multi-level island would be connected. |
| Level fields | Per-level settings: music, ambient light, the name shown on entering. |
| Entity instance size | You want to draw a trigger area as a rectangle in LDtk. |
| Entity instance `iid` | Saving. It is the only identity of a placed entity that stays the same between runs. |
| Entity tags other than `marker` | You want behaviour chosen by a tag ("pickup") instead of by entity type. |
| Enum, Point and array fields | A chest that names its item, a patrol path, a list of dialogue lines. |
| Entity definitions without a tileset | Any invisible entity that should exist at run time. |
| Per-instance display tile | One generic "prop" definition showing a different tile per instance. *Not verified against the library.* |
| Tile flips | An auto-rule allows flipping. |
| Tile alpha, layer opacity | Translucent overlays, such as a canopy or fog layer. |
| IntGrid values | Terrain kinds: slow mud, water, ice. The numbers are in the file and are not read. |
| Layer pixel offset | A layer shifted by a few pixels in LDtk. *Not verified.* |
| Tileset spacing and padding, when finding an entity's tile id | A tileset image with gaps between tiles. Entities would get the wrong hitboxes. |
| Hitboxes on tiles of a pure AutoLayer | You split rules from the IntGrid layer they read. |
| Level background colour and image | Any level that does not cover its whole area with tiles. |
