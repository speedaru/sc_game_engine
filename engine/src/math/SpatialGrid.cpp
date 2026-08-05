#include <pch.h>
#include <engine/math/SpatialGrid.h>
#include <engine/utils/logging.h>

namespace sc::math {
	void SpatialGrid::InsertEntity(entity ent, const Rect& entBox) {
		auto cells = GetCellsFromBox(entBox);

		// insert entity into each grid cell that it occupies
		for (const auto& cell : cells) {
			m_grid[cell].push_back(ent);
		}
	}
	
	auto SpatialGrid::Query(const Rect& box) -> V {
		auto cells = GetCellsFromBox(box);
		V results;

		for (const auto& cell : cells) {
			if (m_grid.contains(cell)) {
				const auto& entitiesInCell = m_grid[cell];
				results.insert(results.end(), entitiesInCell.begin(), entitiesInCell.end());
			}
		}

		// remove duplicates
		std::sort(results.begin(), results.end());
		results.erase(std::unique(results.begin(), results.end()), results.end());

		return results;
	}

	void SpatialGrid::Clear() {
		m_grid.clear();
	}

	auto SpatialGrid::GetCellsFromBox(const Rect& box) -> std::vector<K> {
		std::vector<K> out;

		// calc cell that contains top left coord of the box
		auto topLeftPos = box.position;
		K topLeftCell = { 
			.row = (uint32_t)std::floor(topLeftPos.y / m_cellSize),
			.col = (uint32_t)std::floor(topLeftPos.x / m_cellSize)
		};

		auto bottomRightPos = box.position + box.size;
		K bottomRightCell = {
			.row = (uint32_t)std::ceil(bottomRightPos.y / m_cellSize),
			.col = (uint32_t)std::ceil(bottomRightPos.x / m_cellSize)
		};

		uint32_t xCells = bottomRightCell.col - topLeftCell.col;
		uint32_t yCells = bottomRightCell.row - topLeftCell.row;

		// add all cells in between
		for (uint32_t y = 0; y < yCells; y++) {
			for (uint32_t x = 0; x < xCells; x++) {
				out.push_back(GridCell{
					.row = topLeftCell.row + y,
					.col = topLeftCell.col + x
				});
			}
		}

		return out;
	}

}