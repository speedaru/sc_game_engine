#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <unordered_map>

#include <SFML/Graphics/Rect.hpp>
#include <entt/entity/entity.hpp>

namespace sc::math {
	struct GridCell {
		uint32_t row;
		uint32_t col;

		bool operator==(GridCell other) const { return row == other.row && col == other.col; }
	};

	struct GridCellHash {
		uint32_t cellsPerRow;

		GridCellHash(uint32_t width) : cellsPerRow(width) {}

		size_t operator()(const GridCell& cell) const {
			return static_cast<size_t>(cell.row) * cellsPerRow + cell.col;
		}
	};

	// contains a hashmap of grid cells and which entities exist in those cells
	class SpatialGrid {
		using entity = entt::entity; // unique entity handle
	public:
		// 
		using K = GridCell;
		using V = std::vector<entity>;
		using Rect = sf::FloatRect; // rect type of the entity box

		SpatialGrid(uint32_t cellSize, uint32_t cellsPerRow)
			: m_cellSize(cellSize),
			m_cellsPerRow(cellsPerRow),
			m_grid(0, GridCellHash(m_cellsPerRow)) {}

		void InsertEntity(entity ent, const Rect& entBox);

		// request a list of entities inside this box
		auto Query(const Rect& box) -> V;

		void Clear();

	private:
		// get cells that contain the box
		auto GetCellsFromBox(const Rect& box) -> std::vector<K>;

	private:
		uint32_t m_cellSize; // cell size of each cell in the grid
		uint32_t m_cellsPerRow; // numbers of cells in 1 row (used to calculate cell id for a GridCell)
		std::unordered_map<K, V, GridCellHash> m_grid;
	};
}
