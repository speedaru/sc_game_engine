#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <LDtkLoader/Tileset.hpp>

#include <engine/ecs/Components.h>

namespace game::world_loader {
	// hitboxes per tile, parsed once.
	//
	// the same tile id repeats hundreds of times across a level, and re-parsing its custom
	// data json for every single instance was most of the loader's work.
	//
	// load time only: collision_baker asks for a tile's hitboxes and feeds them to
	// CollisionLayerBuilder::AddBox. nothing consults this at runtime
	class TileColliderCache {
	public:
		// empty if the tile has no custom data or no hitboxes in it
		const std::vector<sc::math::Hitbox>& Get(const ldtk::Tileset& tileset, int tileId);

	private:
		// tileset uid and tile id are both ints, so one map covers every tileset without
		// nesting. tile ids are only unique *within* a tileset, so the uid has to be part
		// of the key
		static uint64_t MakeKey(int tilesetUid, int tileId) {
			return (static_cast<uint64_t>(static_cast<uint32_t>(tilesetUid)) << 32)
				| static_cast<uint32_t>(tileId);
		}

		std::unordered_map<uint64_t, std::vector<sc::math::Hitbox>> m_cache;
	};
}
