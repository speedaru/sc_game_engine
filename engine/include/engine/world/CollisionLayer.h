#pragma once
#include <cstdint>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <engine/math/CellRange.h>
#include <engine/math/Hitbox.h>
#include <engine/physics/ColliderRef.h>

namespace sc::world {
	// the collision geometry of one grid cell, in cell local coordinates
	struct TileShape {
		std::vector<math::Hitbox> boxes;
		uint32_t flags = 0;

		bool operator==(const TileShape& other) const = default;
	};

	// static level geometry as a uniform grid of shape indices
	// cells hold a uint16_t index into a deduped shape palette rather than their own
	// box list to avoid information duplication
	class CollisionLayer {
	public:
		static constexpr uint16_t EMPTY_SHAPE = 0;

		// an empty layer. Level default constructs one and the loader move assigns the result into it
		CollisionLayer() : CollisionLayer({ 0.f, 0.f }, 1u, math::GridSize{ 0u, 0u }) {}

		CollisionLayer(sf::Vector2f origin, uint32_t cellSize, math::GridSize size);

		void QueryArea(const sf::FloatRect& area, physics::CandidateBuffer& out) const;

		uint16_t GetCell(uint32_t row, uint32_t col) const;

		// out of range coordinates are ignored with a warning
		void SetCell(uint32_t row, uint32_t col, uint16_t shapeIndex);

		uint16_t AddShape(TileShape&& shape);

		const TileShape& GetShape(uint16_t index) const;
		uint32_t GetShapeCount() const { return static_cast<uint32_t>(m_shapes.size()); }

		uint32_t CellIndex(uint32_t row, uint32_t col) const;
		sf::Vector2f CellOrigin(uint32_t row, uint32_t col) const;
		sf::FloatRect CellRect(uint32_t row, uint32_t col) const;

		sf::Vector2f GetOrigin() const { return m_origin; }
		uint32_t GetCellSize() const { return m_cellSize; }
		math::GridSize GetSize() const { return m_size; }

		math::CellRange GetCellsFromBox(const sf::FloatRect& area) const;

	private:
		sf::Vector2f m_origin; // world position of cell (0, 0)
		uint32_t m_cellSize;
		math::GridSize m_size;

		std::vector<uint16_t> m_cells; // rows * cols, contains indexes into m_shapes
		std::vector<TileShape> m_shapes; // deduped palette, index EMPTY_SHAPE is an empty shape
	};
}
