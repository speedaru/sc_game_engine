#pragma once
#include <string>
#include <vector>
#include <memory>

#include <SFML/System/Vector2.hpp>

#include <engine/world/ILevelLayer.h>
#include <engine/math/SpatialGrid.h>

namespace sc::world {
	struct LevelId {
		const std::string name;
		int32_t uid;
	};

	// stores layers in bottom to top order
	class Level {
	public:
		static constexpr uint32_t DEFAULT_SPATIAL_GRID_CELL_SIZE = 64u;

		// cell size: cell size for spatial grid
		Level(const LevelId& id, sf::Vector2i size, uint32_t cellSize = DEFAULT_SPATIAL_GRID_CELL_SIZE)
			: m_id(id), m_size(size), m_spatialGrid(cellSize , size.x / cellSize) {}

		// adds layers in bottom to top
		void AddLayer(std::unique_ptr<ILevelLayer> layer);

		const LevelId& GetId() const { return m_id; }
		sf::Vector2i GetSize() const { return m_size; }

		math::SpatialGrid& GetSpatialGrid() { return m_spatialGrid; }

		// expose layer stack for renderer
		const std::vector<std::unique_ptr<ILevelLayer>>& GetLayers() const { return m_layers; }

	private:
		LevelId m_id;
		sf::Vector2i m_size;
		math::SpatialGrid m_spatialGrid;
		std::vector<std::unique_ptr<ILevelLayer>> m_layers;
	};
}
