#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>

#include <engine/graphics/TileSet.h>

namespace fs = std::filesystem;

namespace sc::graphics {
	// Centralizes ownership of TileSet assets so the same tileset file
	// is never loaded from disk more than once.
	class TileSetManager {
	public:
		// returns the cached TileSet for this file if one exists, otherwise loads and caches it
		std::shared_ptr<TileSet> GetOrLoad(const fs::path& file, uint32_t tileSize);

		void Clear();

	private:
		std::unordered_map<std::string, std::shared_ptr<TileSet>> m_cache;
	};
}
