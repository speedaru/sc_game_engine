#pragma once
#include <cstdint>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <engine/math/Grid.hpp>
#include <engine/math/Hitbox.h>
#include <engine/world/CollisionLayer.h>

namespace sc::world {
	// accumulates world space collision boxes from any number of sources and bakes them
	// into one CollisionLayer
	class CollisionLayerBuilder {
	public:
		CollisionLayerBuilder(sf::Vector2f origin, uint32_t cellSize, math::GridSize size);

		void AddBox(const sf::FloatRect& worldBox, uint32_t flags = 0);

		CollisionLayer Build();

		uint32_t GetCellSize() const { return m_cellSize; }
		math::GridSize GetSize() const { return m_size; }

	private:
		sf::Vector2f m_origin;
		uint32_t m_cellSize;
		math::GridSize m_size;

		// per cell, pre dedup. parallel arrays rather than a vector<TileShape> because the
		// flags accumulate by OR while the boxes accumulate by append
		std::vector<std::vector<math::Hitbox>> m_pending;
		std::vector<uint32_t> m_flags;
	};
}
