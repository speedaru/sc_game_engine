#pragma once
#include <engine/world/CollisionLayer.h>

namespace ldtk { class Level; }

namespace game::world_loader {
	class TileColliderCache;

	// create a CollisionLayer from a ldtkLevel
	sc::world::CollisionLayer BakeLevelCollision(const ldtk::Level& ldtkLevel, TileColliderCache& tileColliders);
}
