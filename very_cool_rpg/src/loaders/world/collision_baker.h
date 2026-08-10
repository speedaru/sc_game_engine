#pragma once
#include <cstdint>

#include <LDtkLoader/Layer.hpp>
#include <LDtkLoader/Level.hpp>

#include <engine/world/CollisionLayer.h>
#include <engine/world/CollisionLayerBuilder.h>

namespace game::world_loader {
	class TileColliderCache;

	// turns a level's LDtk collision sources into one baked CollisionLayer.
	//
	// this is where the LDtk side of static collision ends. sources author at different
	// resolutions - 16px tileset custom data and an 8px IntGrid - and they all funnel
	// through CollisionLayerBuilder::AddBox, which clips whatever it is handed to the cells
	// it overlaps. the sources never learn about each other
	sc::world::CollisionLayer BakeLevelCollision(const ldtk::Level& ldtkLevel, TileColliderCache& tileColliders);

	// hitboxes authored in tileset tile custom data. the primary source: collision follows
	// the art, so painting a wall tile anywhere makes it solid with no extra authoring
	void BakeTileLayer(const ldtk::Layer& ldtkLayer, TileColliderCache& tileColliders, sc::world::CollisionLayerBuilder& out);

	// hand painted IntGrid values, for precision the art grid can't express: a doorway
	// notch, a ledge lip, an invisible wall
	void BakeIntGridLayer(const ldtk::Layer& ldtkLayer, sc::world::CollisionLayerBuilder& out);
}
