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
		std::vector<entity> results;
		results.reserve(32);
		Query(box, results);
		return results;
	}

	void SpatialGrid::Query(const Rect& box, std::vector<entity>& out) const {
		const CellRange range = GetCellsFromBox(box);

		// append so callers can reuse one buffer across frames
		const size_t start = out.size();

		for (auto row = range.minRow; row < range.maxRow; row++) {
			for (auto col = range.minCol; col < range.maxCol; col++) {
				const auto& entities = m_grid.GetCell(row, col);
				out.insert(out.end(), entities.begin(), entities.end());
			}
		}

		// remove duplicates because 1 entity can be in multiple cells
		std::sort(out.begin() + start, out.end());
		out.erase(std::unique(out.begin() + start, out.end()), out.end());
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
		// the spatial grid always starts at the world origin
		return CellsFromBox(box, { 0.f, 0.f }, static_cast<float>(m_cellSize), m_size);
	}

}