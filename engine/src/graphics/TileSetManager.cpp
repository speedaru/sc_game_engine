#include <pch.h>
#include <engine/graphics/TileSetManager.h>

namespace sc::graphics {
	std::shared_ptr<TileSet> TileSetManager::GetOrLoad(const std::filesystem::path& file, uint32_t tileSize) {
		const std::string key = std::filesystem::weakly_canonical(file).string();

		auto it = m_cache.find(key);
		if (it != m_cache.end()) {
			return it->second;
		}

		auto tileSet = std::make_shared<TileSet>(file, tileSize);
		m_cache.emplace(key, tileSet);
		return tileSet;
	}

	void TileSetManager::Clear() {
		m_cache.clear();
	}
}
