#include <pch.h>

#include <engine/math/CellRange.h>

namespace sc::math{
	bool CellRange::Intersects(const CellRange& other) const {
		if (IsEmpty() || other.IsEmpty()) return false;

		// half-open: touching edges (maxRow == other.minRow) share no cells
		return minRow < other.maxRow && maxRow > other.minRow &&
			minCol < other.maxCol && maxCol > other.minCol;
	}

	bool CellRange::GetIntersection(const CellRange& other, CellRange& result) const {
		result = CellRange{
			std::max(minRow, other.minRow),
			std::max(minCol, other.minCol),
			std::min(maxRow, other.maxRow),
			std::min(maxCol, other.maxCol)
		};

		// disjoint ranges leave max < min, which IsEmpty() catches
		return !result.IsEmpty();
	}

	void CellRange::Difference(const CellRange& other, const std::function<void(const CellRange&)>& func) const {
		if (IsEmpty()) return;

		CellRange intersection;
		if (!GetIntersection(other, intersection)) {
			// nothing shared -> whole range is the difference
			func(*this);
			return;
		}

		// rows above the intersection, full width
		if (minRow < intersection.minRow) {
			func(CellRange{ minRow, minCol, intersection.minRow, maxCol });
		}

		// rows below the intersection, full width
		if (intersection.maxRow < maxRow) {
			func(CellRange{ intersection.maxRow, minCol, maxRow, maxCol });
		}

		// cols left of the intersection, only across the rows it spans
		if (minCol < intersection.minCol) {
			func(CellRange{ intersection.minRow, minCol, intersection.maxRow, intersection.minCol });
		}

		// cols right of the intersection, only across the rows it spans
		if (intersection.maxCol < maxCol) {
			func(CellRange{ intersection.minRow, intersection.maxCol, intersection.maxRow, maxCol });
		}
	}

	CellRange CellsFromBox(const sf::FloatRect& box, sf::Vector2f origin, float cellSize, GridSize size) {
		const float maxRows = static_cast<float>(size.rows);
		const float maxCols = static_cast<float>(size.cols);

		// grid space, so a grid that doesn't start at the world origin still works
		const float localX = box.position.x - origin.x;
		const float localY = box.position.y - origin.y;

		// stay in float space so we dont underflow
		const float minCol = std::floor(localX / cellSize);
		const float minRow = std::floor(localY / cellSize);
		const float maxCol = std::ceil((localX + box.size.x) / cellSize);
		const float maxRow = std::ceil((localY + box.size.y) / cellSize);

		// clamp to world space coordinates
		return CellRange{
			.minRow = static_cast<uint32_t>(std::clamp(minRow, 0.f, maxRows)),
			.minCol = static_cast<uint32_t>(std::clamp(minCol, 0.f, maxCols)),
			.maxRow = static_cast<uint32_t>(std::clamp(maxRow, 0.f, maxRows)),
			.maxCol = static_cast<uint32_t>(std::clamp(maxCol, 0.f, maxCols))
		};
	}
}
