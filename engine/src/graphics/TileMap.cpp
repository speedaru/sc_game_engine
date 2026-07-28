#include <pch.h>
#include <engine/graphics/TileMap.h>
#include <engine/utils/logging.h>

namespace sc::graphics {
	TileMap::TileMap(const std::shared_ptr<TileSet>& tileSet)
		: m_tileSet(tileSet)
	{
        m_vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
	}

	bool TileMap::Load(const MapData& map) {
		if (!m_tileSet) {
			LOG_E("TileMap failed to load because TileSet is not set");
			return false;
		}
		
		uint32_t mapArea = map.width * map.height;
		if (map.tiles.size() != mapArea) {
			LOG_E("TileMap data size doesn't match map dimensions");
			return false;
		}

		uint32_t tileSize = m_tileSet->GetTileSize();
		uint32_t tilesetColumns = m_tileSet->GetGridWidth();

		m_vertices.resize(mapArea * TILE_VERTICES_COUNT );
		for (uint32_t y = 0; y < map.height; ++y) {
			for (uint32_t x = 0; x < map.width; ++x) {
				int32_t tileID = map.tiles[static_cast<size_t>(y) * map.width + x];
				if (tileID == -1) continue;

				uint32_t tsRow = tileID / tilesetColumns;
				uint32_t tsCol = tileID % tilesetColumns;

				sf::IntRect texRect = m_tileSet->GetTileRect(tsRow, tsCol);

				// pointer to vertices where we have to write the triangles
				sf::Vertex* triangles = &m_vertices[(static_cast<size_t>(y) * map.width + x) * TILE_VERTICES_COUNT];

				float fx = static_cast<float>(x * tileSize);
				float fy = static_cast<float>(y * tileSize);
				float fx2 = static_cast<float>((x + 1) * tileSize);
				float fy2 = static_cast<float>((y + 1) * tileSize);

				// calc 4 corners in world space
				sf::Vector2f topLeft(fx, fy);
				sf::Vector2f topRight(fx2, fy);
				sf::Vector2f bottomRight(fx2, fy2);
				sf::Vector2f bottomLeft(fx, fy2);

				// calc 4 corners in texture space
				float tx = static_cast<float>(texRect.position.x);
				float ty = static_cast<float>(texRect.position.y);
				float tw = static_cast<float>(texRect.size.x);
				float th = static_cast<float>(texRect.size.y);

				sf::Vector2f texTopLeft(tx, ty);
				sf::Vector2f texTopRight(tx + tw, ty);
				sf::Vector2f texBottomRight(tx + tw, ty + th);
				sf::Vector2f texBottomLeft(tx, ty + th);

				// top triangle
				/* _.
				 * \|
				**/
				triangles[0].position = topLeft;
				triangles[0].texCoords = texTopLeft;

				triangles[1].position = topRight;
				triangles[1].texCoords = texTopRight;

				triangles[2].position = bottomRight;
				triangles[2].texCoords = texBottomRight;

				// bottom triangle
				/*
				 * |\
				 * |_\
				**/
				triangles[3].position = bottomRight;
				triangles[3].texCoords = texBottomRight;

				triangles[4].position = bottomLeft;
				triangles[4].texCoords = texBottomLeft;

				triangles[5].position = topLeft;
				triangles[5].texCoords = texTopLeft;
			}
		}

		return true;
	}
}
