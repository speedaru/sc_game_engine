#pragma once
#include <vector>
#include <memory>
#include <filesystem>

#include <SFML/Graphics/Rect.hpp>

#include <engine/graphics/Texture2D.h>
namespace fs = std::filesystem;

namespace sc::graphics {
	class TileSet {
	public:
		TileSet(const fs::path& file, uint32_t tileSize);
		TileSet(const std::vector<uint8_t>& data, uint32_t tileSize);

		sf::IntRect GetTileRect(uint32_t row, uint32_t col) const;

		std::shared_ptr<Texture2D> GetTexture() const { return m_texture; }
		uint32_t GetTileSize() const { return m_tileSize; }
		uint32_t GetGridWidth() const { return m_cols; }
		uint32_t GetGridHeight() const { return m_rows; }

	private:
		std::shared_ptr<Texture2D> m_texture;
		uint32_t m_tileSize;
		uint32_t m_cols{ 0 };
		uint32_t m_rows{ 0 };
	};
}
