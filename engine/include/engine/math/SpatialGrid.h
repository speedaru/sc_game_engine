#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <unordered_map>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <entt/entity/entity.hpp>

#include <engine/math/Grid.hpp>
#include <engine/math/CellRange.h>

namespace sc::math {
	// contains a hashmap of grid cells and which entities exist in those cells
	class SpatialGrid {
		using entity = entt::entity; // unique entity handle
	public:
		using Rect = sf::FloatRect; // rect type of the entity box

		SpatialGrid(uint32_t cellSize, GridSize size)
			: m_cellSize(cellSize),
			m_size(size),
			m_grid(size) {}

		void InsertEntity(entity ent, const Rect& entBox);

		void EraseEntity(entity ent);

		void MoveEntity(entity ent, const Rect& newBox);

		// request a list of entities inside this box
		auto Query(const Rect& box) -> std::vector<entity>;

		void Clear();

	private:
		// InsertRange and EraseRange only touch m_grid, not m_entityCells

		void InsertRange(entity ent, const CellRange& range);
		void EraseRange(entity ent, const CellRange& range);

		// get cells that contain the box
		auto GetCellsFromBox(const Rect& box) const -> CellRange;

	private:
		uint32_t m_cellSize; // cell size of each cell in the grid
		GridSize m_size; // grid size in rows and columns
		Grid<std::vector<entity>> m_grid; // tracks which entities occupy a specific cells
		std::unordered_map<entity, CellRange> m_entityCells; // reverse indexing: tracks what cell range an entity occupies
	};
}
