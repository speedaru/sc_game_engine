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
}
