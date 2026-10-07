#pragma once
#include <string>
#include <vector>
#include <memory>

#include <SFML/System/Vector2.hpp>

#include <engine/world/ILevelLayer.h>
#include <engine/world/CollisionLayer.h>
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
		Level(const LevelId& id, const sf::Vector2u& size, uint32_t cellSize = DEFAULT_SPATIAL_GRID_CELL_SIZE)
			: m_id(id),
			m_size(size),
			// round up
			m_spatialGrid(cellSize, math::GridSize{
				.rows = (size.y + cellSize - 1) / cellSize,
				.cols = (size.x + cellSize - 1) / cellSize
			})
		{
		}

		// adds layers in bottom to top
		void AddLayer(std::unique_ptr<ILevelLayer> layer);

		const std::string& GetName() const { return m_id.name; }
		int32_t GetUid() const { return m_id.uid; }
		sf::Vector2u GetSize() const { return m_size; }

		const math::SpatialGrid& GetSpatialGrid() const { return m_spatialGrid; }
		math::SpatialGrid& GetSpatialGrid() { return m_spatialGrid; }

		const CollisionLayer& GetCollisionLayer() const { return m_collisionLayer; }
		CollisionLayer& GetCollisionLayer() { return m_collisionLayer; }

		// the loader bakes the collision layer from all of a level's sources at once and
		// moves the finished result in
		void SetCollisionLayer(CollisionLayer&& collisionLayer) { m_collisionLayer = std::move(collisionLayer); }

		// expose layer stack for renderer
		const std::vector<std::unique_ptr<ILevelLayer>>& GetLayers() const { return m_layers; }

	private:
		LevelId m_id;
		sf::Vector2u m_size;
		math::SpatialGrid m_spatialGrid;
		CollisionLayer m_collisionLayer;
		std::vector<std::unique_ptr<ILevelLayer>> m_layers;
	};
}
