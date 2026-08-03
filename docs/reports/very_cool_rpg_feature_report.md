# `very_cool_rpg` — feature report

Date: 2026-08-03
Scope: `very_cool_rpg/` only (the game). Engine and editor covered only where
directly relevant to what the game does today.

## What it is right now

A single-window SFML app (`main.cpp`) that pushes one `GameplayLayer` and
loads one hardcoded LDtk level (`World_Level_0` from
`assets/world_test1.ldtk`). There's a player character that walks around
with 4-directional movement and collides with tile-authored hitboxes, plus
one other spawned entity type (a dragon) that currently has no behavior.
No combat, no UI, no audio, no animation, no level transitions.

## Features implemented

**World loading & content pipeline**
- Levels, tile layers, and entities are authored entirely in LDtk and loaded
  through a modular loader pipeline (`src/loaders/world_loader.*` +
  `src/loaders/world/`) — see `docs/reports/world_loader_review.md` and its
  follow-up rewrite for the architecture. Concretely this gives the game:
  - Ordinary scrolling tile layers and **Y-sorted tile layers** (tag-driven
    via `_YS_` in the layer name), so tall foreground tiles depth-sort
    against entities correctly.
  - Per-tile collision authored as custom data in the LDtk tileset (not a
    separate collision layer) — a tile with `{"hitboxes": [...]}` custom
    data spawns an invisible `Tile_Collider_x_y` ECS entity with a
    `BoxColliderComponent` at load time.
  - Per-entity hitboxes authored the same way, reusing the entity's display
    tile's custom data, pivot-aligned to the entity's transform origin.
  - `TileSetManager` (engine-level) caches loaded tilesets so the same
    tileset file is never read from disk twice across layers/levels.
- Content-authoring loop: `sc_editor` is a standalone ImGui tool for placing
  those per-tile hitboxes visually (draw/drag/resize boxes on a tile,
  save back into the `.ldtk` file) instead of hand-editing JSON. It shares
  the same `CustomData` schema definition as the game's loader (via a
  parallel `to_json`/`from_json` in `sc_editor/src/CustomData.*`), and
  preserves any custom-data fields it doesn't have UI for yet so it can't
  silently destroy future data added by other tools.

**Entity spawning**
- `EntityFactory` + a small **blueprint** system
  (`src/blueprints/`): each LDtk entity identifier (`"Player"`, `"Dragon"`)
  maps to an `IBlueprint` that assembles the ECS entity's components on
  spawn. Currently registered: `PlayerBlueprint`, `DragonBlueprint`.
- Both blueprints attach a `TransformComponent` + `SpriteComponent` (sourced
  from the entity's LDtk texture/tileset via `LdtkAssemblers`, with a
  path-keyed texture cache) plus `VelocityComponent` and a game-side
  `CharacterController` component. `PlayerBlueprint` additionally tags the
  entity with `PlayerTag` so systems can single it out.

**Movement & input**
- `InputContext`/`InputAction` binds WASD to `MoveUp/Down/Left/Right`
  (`GameplayLayer::OnAttach`).
- `player_input` system reads those actions each frame into the player's
  `CharacterController.direction`.
- `character_movement` system turns that direction into velocity: normalizes
  diagonal input, applies acceleration up to a max speed, and applies
  friction to decelerate to a stop when there's no input.

**Physics / collision**
- Engine-side swept-AABB collision (`engine/math/physics.h/.cpp`,
  `physics_system.cpp`): every dynamic entity (has `Velocity` +
  `BoxCollider`) is swept against every static entity (has `BoxCollider`,
  no `Velocity`) each fixed update, with up to 3 slide iterations per frame
  so entities slide along walls instead of stopping dead on contact.
  Colliders are compound (a component can hold multiple hitbox rects).
- Debug view: `GameplayLayer::OnRender` draws all `BoxColliderComponent`
  hitboxes as translucent red rectangles over the game.

**Rendering**
- Batched 2D quad renderer (`render2d`) driven by `render_system`: iterates
  a level's layer stack bottom-to-top, flushing the Y-sort queue at each
  "depth boundary" layer so Y-sorted tiles interleave correctly with
  entities drawn on entity layers. Camera is a simple 2D camera that centers
  on the player every fixed update (no bounds clamping yet — see below).

## What's explicitly missing

Noting these because "RPG" implies expectations this codebase doesn't meet
yet — not a criticism, just the honest gap list:

- **No AI/behavior for non-player entities.** `DragonBlueprint` gives the
  dragon a `CharacterController` and `VelocityComponent`, but nothing ever
  writes to its `direction` — it's structurally movable but inert. There's
  no AI/behavior system of any kind.
- **No animation.** `SpriteComponent` holds one static texture rect set at
  spawn time; there's no animation component or system anywhere in the repo
  (confirmed via grep). Walking in place looks like sliding.
- **No camera bounds.** `GameplayLayer.cpp` has a literal
  `// TODO: limit camera in map edges` — the camera can currently show past
  the level's edge.
- **No combat, health, or stats** of any kind.
- **No UI/HUD** — no health bar, inventory, dialogue box, menus.
- **No audio** — no sound or music system exists (confirmed via grep).
- **Single hardcoded level, no transitions.** `GameplayLayer::OnRender`
  hardcodes `"World_Level_0"`; `World` supports multiple levels
  (`AddLevel`/`GetLevel`) but nothing in the game ever switches between
  them or spawns the player at a level-entry point.
- **No save/load.**
- **No scripting/trigger system**, though the custom-data rework was
  explicitly built to make this cheap to add later (a new field + handler
  in `loaders/world/CustomData.cpp`, no loader restructuring needed).

## Recommended next steps

Roughly in the order I'd tackle them — each one either unblocks the next or
is currently the single biggest gap between "tech demo" and "RPG":

1. **Sprite animation.** Everything downstream (combat, AI, feedback) reads
   as broken without it, and it's self-contained: an `AnimationComponent`
   (frame list + timer) and a small system that rewrites `SpriteComponent`'s
   `rect` each frame. No loader or ECS architecture changes needed.
2. **Camera bounds clamping.** Small, already flagged with a TODO, and
   removes a visible rough edge immediately.
3. **A basic entity/trigger interaction system**, built on the new
   `CustomData` extensibility: add a `"trigger"` (or similar) custom-data
   field alongside `"hitboxes"`, and a corresponding system that checks
   player-overlap against static trigger entities/tiles. This is the
   foundation multiple later features (level transitions, dialogue,
   pickups) all need, and the loader/editor plumbing for it already exists
   by design.
4. **Level transitions.** `World` already supports multiple levels; the gap
   is purely game-side — an active-level pointer on `GameplayLayer`,
   door/trigger entities (built on #3) that call `World::GetLevel` and
   reposition the player.
5. **Minimal AI for non-player entities.** Even something simple (patrol
   waypoints, or "face and approach the player within range") makes the
   dragon read as a game object instead of set dressing. Natural home is a
   new `systems/` file alongside `character_movement`, since
   `CharacterController` already exists as the shared movement interface —
   an AI system would just write `direction` instead of `player_input`.
6. **Combat.** Depends on #1 (animation feedback) and #5 (something to
   fight) existing first. Start minimal: a health component, a hit/hurtbox
   pass reusing the existing `BoxColliderComponent`/swept-collision
   machinery, and a damage event.
7. **HUD/UI.** Health bar and any player-facing feedback; low urgency until
   there's something (combat, pickups) worth surfacing.
8. **Audio.** No blockers, purely a matter of prioritization — cheapest to
   add whenever it's wanted, highest impact once movement/combat feel right
   (footsteps, hit sounds, ambient loops).

Save/load and a full dialogue/quest system are worth deferring until the
above land — they're substantially easier to design correctly once there's
real game state (multiple levels, combat, triggers) to persist or drive
dialogue from.
