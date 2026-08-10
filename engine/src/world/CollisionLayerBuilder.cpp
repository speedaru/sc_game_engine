#include <pch.h>
#include <engine/world/CollisionLayerBuilder.h>

#include <engine/math/CellRange.h>
#include <engine/utils/logging.h>

namespace sc::world {
	namespace {
		constexpr float EPSILON = 0.001f;

		bool Overlaps(const math::Hitbox& a, const math::Hitbox& b) {
			return a.offset.x < b.offset.x + b.size.x && a.offset.x + a.size.x > b.offset.x &&
				a.offset.y < b.offset.y + b.size.y && a.offset.y + a.size.y > b.offset.y;
		}

		// canonical order, so two cells that ended up with the same geometry from different
		// sources in a different order still dedup to one palette entry
		bool Before(const math::Hitbox& a, const math::Hitbox& b) {
			if (a.offset.y != b.offset.y) return a.offset.y < b.offset.y;
			if (a.offset.x != b.offset.x) return a.offset.x < b.offset.x;
			if (a.size.y != b.size.y) return a.size.y < b.size.y;
			return a.size.x < b.size.x;
		}

		// checks if boxes cover the whole area of the cell
		bool CoversCell(const std::vector<math::Hitbox>& boxes, float cellSize) {
			float area = 0.f;
			for (const math::Hitbox& box : boxes) {
				area += box.size.x * box.size.y;
			}

			if (std::abs(area - cellSize * cellSize) > EPSILON) return false;

			for (size_t i = 0; i < boxes.size(); i++) {
				for (size_t j = i + 1; j < boxes.size(); j++) {
					if (Overlaps(boxes[i], boxes[j])) return false;
				}
			}

			return true;
		}
	}

	CollisionLayerBuilder::CollisionLayerBuilder(sf::Vector2f origin, uint32_t cellSize, math::GridSize size)
		: m_origin(origin),
		m_cellSize(cellSize > 0u ? cellSize : 1u),
		m_size(size),
		m_pending(static_cast<size_t>(size.rows)* size.cols),
		m_flags(static_cast<size_t>(size.rows)* size.cols, 0u)
	{
		if (cellSize == 0u) {
			LOG_E("collision layer builder given a cell size of 0, falling back to 1");
		}
	}

	void CollisionLayerBuilder::AddBox(const sf::FloatRect& worldBox, uint32_t flags) {
		if (worldBox.size.x <= 0.f || worldBox.size.y <= 0.f) return;

		const float boxLeft = worldBox.position.x;
		const float boxTop = worldBox.position.y;
		const float boxRight = boxLeft + worldBox.size.x;
		const float boxBottom = boxTop + worldBox.size.y;

		const float cellSize = static_cast<float>(m_cellSize);
		const math::CellRange range = math::CellsFromBox(worldBox, m_origin, cellSize, m_size);

		for (uint32_t row = range.minRow; row < range.maxRow; row++) {
			for (uint32_t col = range.minCol; col < range.maxCol; col++) {
				const sf::Vector2f cellOrigin{
					m_origin.x + col * cellSize,
					m_origin.y + row * cellSize
				};

				// clip to cell so this box can't overflow outside of cell
				const float left = std::max(boxLeft, cellOrigin.x);
				const float top = std::max(boxTop, cellOrigin.y);
				const float right = std::min(boxRight, cellOrigin.x + cellSize);
				const float bottom = std::min(boxBottom, cellOrigin.y + cellSize);

				// ensure doesn't floats dont overlap with edges
				if (right - left <= EPSILON || bottom - top <= EPSILON) continue;

				const size_t index = static_cast<size_t>(row) * m_size.cols + col;

				// ADD box
				m_pending[index].emplace_back(
					left - cellOrigin.x,
					top - cellOrigin.y,
					right - left,
					bottom - top
				);
				// OR flags
				m_flags[index] |= flags;
			}
		}
	}

	CollisionLayer CollisionLayerBuilder::Build() {
		CollisionLayer layer(m_origin, m_cellSize, m_size);

		const float cellSize = static_cast<float>(m_cellSize);

		for (uint32_t row = 0; row < m_size.rows; row++) {
			for (uint32_t col = 0; col < m_size.cols; col++) {
				const size_t index = static_cast<size_t>(row) * m_size.cols + col;
				std::vector<math::Hitbox>& boxes = m_pending[index];

				if (boxes.empty()) continue;

				// sort so we can remove duplicate boxes, and also so we can deduplicate shapes
				// regardless of the order in which we called AddBox()
				std::sort(boxes.begin(), boxes.end(), Before);
				boxes.erase(std::unique(boxes.begin(), boxes.end()), boxes.end());

				// if boxes cover the entire cell remove boxes and put 1 single box that covers the entire cell exactly
				if (CoversCell(boxes, cellSize)) {
					boxes.clear();
					boxes.emplace_back(0.f, 0.f, cellSize, cellSize);
				}

				layer.SetCell(row, col, layer.AddShape(TileShape{
					.boxes = std::move(boxes),
					.flags = m_flags[index]
				}));
			}
		}

		LOG_D("baked collision layer: %ux%u cells at %upx, %u distinct shapes",
			m_size.rows, m_size.cols, m_cellSize, layer.GetShapeCount());

		return layer;
	}
}
