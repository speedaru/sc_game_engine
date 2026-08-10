#include <pch.h>
#include <loaders/world/TileColliderCache.h>

#include <loaders/world/HitboxUtils.h>

namespace math = sc::math;

namespace game::world_loader {
	const std::vector<math::Hitbox>& TileColliderCache::Get(const ldtk::Tileset& tileset, int tileId) {
		const uint64_t key = MakeKey(tileset.uid, tileId);

		auto it = m_cache.find(key);
		if (it != m_cache.end()) return it->second;

		// misses are cached too, as an empty vector: most tiles in a tileset have no
		// custom data at all, and without this every one of them re-checks every frame
		// of the load
		const std::string& customData = tileset.getTileCustomData(tileId);
		auto [entry, inserted] = m_cache.emplace(key, ParseHitboxesFromCustomData(customData));

		return entry->second;
	}
}
