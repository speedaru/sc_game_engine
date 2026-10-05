#include <pch.h>
#include <engine/graphics/TileSet.h>
#include <engine/utils/logging.h>

namespace sc::graphics {
	TileSet::TileSet(const fs::path& file, uint32_t tileSize)
		: m_tileSize(tileSize)
	{
		m_texture = CreateTexture2D(file);
		m_cols = m_texture->GetWidth() / m_tileSize;
		m_rows = m_texture->GetHeight() / m_tileSize;
	}

	TileSet::TileSet(const std::vector<uint8_t>& data, uint32_t tileSize)
		: m_tileSize(tileSize)
	{
		m_texture = CreateTexture2D(data);
		m_cols = m_texture->GetWidth() / m_tileSize;
		m_rows = m_texture->GetHeight() / m_tileSize;
	}

	sf::IntRect TileSet::GetTileRect(uint32_t row, uint32_t col) const {
		if (row >= m_rows || col >= m_cols) {
			LOG_W("TileSet requested tile out of bouds row: %u col: %u", row, col);
			return sf::IntRect({ 0, 0 }, { (int)m_tileSize, (int)m_tileSize });
		}

		int x = col * m_tileSize;
		int y = row * m_tileSize;

		return sf::IntRect({ x, y }, { (int)m_tileSize, (int)m_tileSize });
	}
}
