#pragma once
#include <vector>
#include <memory>

#include <SFML/Graphics/VertexArray.hpp>

#include <engine/graphics/TileSet.h>

namespace sc::graphics {
	class TileMap {
	public:
		constexpr static const uint32_t TILE_VERTICES_COUNT = 6;

		TileMap() = default;
		TileMap(const std::shared_ptr<TileSet>& tileSet);
		
		bool Load(const std::vector<int32_t>& tiles, uint32_t mapWidth, uint32_t mapHeight);

		const sf::VertexArray& GetVertices() const { return m_vertices; }
		const std::shared_ptr<TileSet>& GetTileSet() const { return m_tileSet; }

	private:
		std::shared_ptr<TileSet> m_tileSet;
		sf::VertexArray m_vertices;
	};
}
