#include <pch.h>

#include <engine/math/SpatialGrid.h>
#include <engine/utils/logging.h>

namespace sc::math {
	void SpatialGrid::InsertEntity(entity ent, const Rect& entBox) {
		// ensure the entity doesnt already exist
		if (m_entityCells.contains(ent)) {
			LOG_W("trying to insert entity %u into spatial grid but it already exists", static_cast<uint32_t>(ent));
			return;
		}

		CellRange range = GetCellsFromBox(entBox);
		InsertRange(ent, range);

		// update entity cells map
		m_entityCells[ent] = range;
	}

	void SpatialGrid::EraseEntity(entity ent) {
		if (!m_entityCells.contains(ent)) {
			LOG_W("trying to erase an entity that doesn't exist in spatial grid: %u", static_cast<uint32_t>(ent));
			return;
		}

		// get cell range so we can query the grid map
		CellRange range = m_entityCells[ent];

		// erase from entity cells map
		m_entityCells.erase(ent);

		EraseRange(ent, range);
	}

	void SpatialGrid::MoveEntity(entity ent, const Rect& newBox) {
		if (!m_entityCells.contains(ent)) {
			LOG_W("trying to move an entity that doesn't exist in spatial grid: %u", static_cast<uint32_t>(ent));
			return;
		}

		CellRange newRange = GetCellsFromBox(newBox);
		CellRange prevRange = m_entityCells[ent];

		if (newRange == prevRange) {
			return;
		}

		// calc which cell range we need to erase entity from
		// and which one we need to insert entity into
		if (newRange.Intersects(prevRange)) {
			// optimization: only erase and insert cells that dont overlap
			prevRange.Difference(newRange, [this, ent](const CellRange& range) { EraseRange(ent, range); });
			newRange.Difference(prevRange, [this, ent](const CellRange& range) { InsertRange(ent, range); });
		}
		else {
			// no intersection so we can erase and reinsert everything
			EraseRange(ent, prevRange);
			InsertRange(ent, newRange);
		}

		// update entity cells map to new range
		m_entityCells[ent] = newRange;
	}
	
	auto SpatialGrid::Query(const Rect& box) -> std::vector<entity> {
		CellRange range = GetCellsFromBox(box);
		std::vector<entity> results;
		results.reserve(32);

		for (auto row = range.minRow; row < range.maxRow; row++) {
			for (auto  col = range.minCol; col < range.maxCol; col++) {
				const auto& entities = m_grid.GetCell(row, col);
				results.insert(results.end(), entities.begin(), entities.end());
			}
		}

		// remove duplicates because 1 entity can be in multiple cells
		std::sort(results.begin(), results.end());
		results.erase(std::unique(results.begin(), results.end()), results.end());

		return results;
	}

	void SpatialGrid::Clear() {
		m_grid.Clear();
		m_entityCells.clear();
	}

	const std::vector<entt::entity>& SpatialGrid::GetCellEntities(uint32_t row, uint32_t col) const {
		return m_grid.GetCell(row, col);
	}

	std::optional<CellRange> SpatialGrid::FindEntityCells(entt::entity ent) const {
		auto it = m_entityCells.find(ent);
		if (it != m_entityCells.end()) {
			return it->second;
		}

		// not found
		return std::nullopt;
	}

	void SpatialGrid::InsertRange(entity ent, const CellRange& range) {
		// update grid map
		for (auto row = range.minRow; row < range.maxRow; row++) {
			for (auto col = range.minCol; col < range.maxCol; col++) {
				// add entity to each cell
				m_grid.GetCell(row, col).push_back(ent);
			}
		}
	}

	void SpatialGrid::EraseRange(entity ent, const CellRange& range) {
		// erase from grid
		for (auto row = range.minRow; row < range.maxRow; row++) {
			for (auto col = range.minCol; col < range.maxCol; col++) {
				// ensure entity exists in cell
				auto& entities = m_grid.GetCell(row, col);
				auto it = std::find(entities.begin(), entities.end(), ent);
				if (it != entities.end()) {
					entities.erase(it);
				}
			}
		}
	}

	auto SpatialGrid::GetCellsFromBox(const Rect& box) const -> CellRange {
		const float cellSize = static_cast<float>(m_cellSize);
		const float maxRows = static_cast<float>(m_size.rows);
		const float maxCols = static_cast<float>(m_size.cols);

		// stay in float space until after clamping: a box can legitimately fall
		// outside the level (MoveAndResolve stretches query bounds by the movement
		// delta), and casting a negative float to uint32_t is undefined behaviour
		float minCol = std::floor(box.position.x / cellSize);
		float minRow = std::floor(box.position.y / cellSize);
		float maxCol = std::ceil((box.position.x + box.size.x) / cellSize);
		float maxRow = std::ceil((box.position.y + box.size.y) / cellSize);

		// cells outside the level hold nothing, so clamping loses no information.
		// maxes clamp to rows/cols (not -1) because the range is half-open
		return CellRange{
			.minRow = static_cast<uint32_t>(std::clamp(minRow, 0.f, maxRows)),
			.minCol = static_cast<uint32_t>(std::clamp(minCol, 0.f, maxCols)),
			.maxRow = static_cast<uint32_t>(std::clamp(maxRow, 0.f, maxRows)),
			.maxCol = static_cast<uint32_t>(std::clamp(maxCol, 0.f, maxCols))
		};
	}

}