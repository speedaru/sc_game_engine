#pragma once
#include <cstdint>

#include <LDtkLoader/Layer.hpp>
#include <LDtkLoader/Level.hpp>

#include <engine/world/CollisionLayer.h>
#include <engine/world/CollisionLayerBuilder.h>

namespace game::world_loader {
	class TileColliderCache;

	void BakeTileLayer(const ldtk::Layer& ldtkLayer, TileColliderCache& tileColliders, sc::world::CollisionLayerBuilder& out);

	void BakeIntGridLayer(const ldtk::Layer& ldtkLayer, TileColliderCache& tileColliders, sc::world::CollisionLayerBuilder& out);

	// create a CollisionLayer from a ldtkLevel
	sc::world::CollisionLayer BakeLevelCollision(const ldtk::Level& ldtkLevel, TileColliderCache& tileColliders);
}
