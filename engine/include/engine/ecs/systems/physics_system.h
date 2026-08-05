#pragma once
#include <engine/ecs/Registry.h>
#include <engine/math/SpatialGrid.h>

namespace sc::ecs::physics_system  {
	// called once after loading a level to build the spatial grid
	void BuildSpatialGrid(Registry& registry, math::SpatialGrid& spatialGrid);

	// called every frame in OnFixedUpdate to apply the physics
	void UpdateKinematics(Registry& registry, math::SpatialGrid& grid, float timeStep);
}
