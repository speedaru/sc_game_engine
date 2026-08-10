#pragma once
#include <cstdint>
#include <vector>
#include <functional>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <engine/math/Grid.hpp>

namespace sc::math {
	// half-open range: rows [minRow, maxRow) x cols [minCol, maxCol),
	// matching GetCellsFromBox's floor/ceil and every `< max` loop
	struct CellRange {
		uint32_t minRow = 0;
		uint32_t minCol = 0;
		uint32_t maxRow = 0;
		uint32_t maxCol = 0;

		bool operator==(const CellRange& rhs) const { return minRow == rhs.minRow && minCol == rhs.minCol && maxRow == rhs.maxRow && maxCol == rhs.maxCol; }

		bool IsEmpty() const { return minRow >= maxRow || minCol >= maxCol; }

		bool Intersects(const CellRange& other) const;

		// this AND other
		bool GetIntersection(const CellRange& other, CellRange& result) const;

		// this \ other, emitted as up to 4 disjoint sub-ranges
		void Difference(const CellRange& other, const std::function<void(const CellRange&)>& func) const;
	};

	// half-open cell range covering box clamped to size
	// origin is the world position of cell (0, 0)
	CellRange CellsFromBox(const sf::FloatRect& box, sf::Vector2f origin, float cellSize, GridSize size);
}
