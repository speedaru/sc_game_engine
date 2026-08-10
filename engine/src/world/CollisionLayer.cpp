#include <pch.h>
#include <engine/world/CollisionLayer.h>

#include <engine/utils/logging.h>

namespace sc::world {
	CollisionLayer::CollisionLayer(sf::Vector2f origin, uint32_t cellSize, math::GridSize size)
		: m_origin(origin),
		m_cellSize(cellSize > 0u ? cellSize : 1u),
		m_size(size),
		m_cells(static_cast<size_t>(size.rows) * size.cols, EMPTY_SHAPE)
	{
		// the first shape is an empty shape with no collisions
		m_shapes.emplace_back();
	}

	void CollisionLayer::QueryArea(const sf::FloatRect& area, physics::CandidateBuffer& out) const {
		const math::CellRange range = GetCellsFromBox(area);

		for (uint32_t row = range.minRow; row < range.maxRow; row++) {
			for (uint32_t col = range.minCol; col < range.maxCol; col++) {
				const uint32_t index = CellIndex(row, col);
				const uint16_t shapeIndex = m_cells[index];

				// skip empty cells
				if (shapeIndex == EMPTY_SHAPE) continue;

				const sf::Vector2f cellOrigin = CellOrigin(row, col);

				for (const math::Hitbox& box : m_shapes[shapeIndex].boxes) {
					out.push_back(physics::ColliderRef{
						.box = sf::FloatRect{ cellOrigin + box.offset, box.size },
						.entity = entt::null,
						.tileIndex = index
					});
				}
			}
		}
	}

	uint16_t CollisionLayer::GetCell(uint32_t row, uint32_t col) const {
		if (row >= m_size.rows || col >= m_size.cols) return EMPTY_SHAPE;

		return m_cells[CellIndex(row, col)];
	}

	void CollisionLayer::SetCell(uint32_t row, uint32_t col, uint16_t shapeIndex) {
		if (row >= m_size.rows || col >= m_size.cols) {
			LOG_W("SetCell out of range: (%u, %u) in a %ux%u collision layer", row, col, m_size.rows, m_size.cols);
			return;
		}

		if (shapeIndex >= m_shapes.size()) {
			LOG_W("SetCell with shape index %u but the palette only has %zu shapes", shapeIndex, m_shapes.size());
			return;
		}

		m_cells[CellIndex(row, col)] = shapeIndex;
	}

	uint16_t CollisionLayer::AddShape(TileShape&& shape) {
		// try to use existing shapre
		for (size_t i = 0; i < m_shapes.size(); i++) {
			if (m_shapes[i] == shape) return static_cast<uint16_t>(i);
		}

		if (m_shapes.size() > std::numeric_limits<uint16_t>::max()) {
			LOG_E("collision shape palette is full (%zu shapes), reusing the empty shape."
				" the cell size is almost certainly too coarse", m_shapes.size());
			return EMPTY_SHAPE;
		}

		m_shapes.push_back(std::move(shape));
		return static_cast<uint16_t>(m_shapes.size() - 1);
	}

	const TileShape& CollisionLayer::GetShape(uint16_t index) const {
		if (index >= m_shapes.size()) return m_shapes[EMPTY_SHAPE];

		return m_shapes[index];
	}

	uint32_t CollisionLayer::CellIndex(uint32_t row, uint32_t col) const {
		return row * m_size.cols + col;
	}

	sf::Vector2f CollisionLayer::CellOrigin(uint32_t row, uint32_t col) const {
		const float cellSize = static_cast<float>(m_cellSize);

		return { m_origin.x + col * cellSize, m_origin.y + row * cellSize };
	}

	sf::FloatRect CollisionLayer::CellRect(uint32_t row, uint32_t col) const {
		const float cellSize = static_cast<float>(m_cellSize);

		return sf::FloatRect{ CellOrigin(row, col), { cellSize, cellSize } };
	}

	math::CellRange CollisionLayer::GetCellsFromBox(const sf::FloatRect& area) const {
		return math::CellsFromBox(area, m_origin, static_cast<float>(m_cellSize), m_size);
	}
}
