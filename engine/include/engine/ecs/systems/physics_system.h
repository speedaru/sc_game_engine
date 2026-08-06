#pragma once
#include <engine/ecs/Registry.h>
#include <engine/math/SpatialGrid.h>

namespace sc::ecs {
	struct TransformComponent;
	struct BoxColliderComponent;
}

namespace sc::ecs::physics_system  {
	// whether an entity belongs in the spatial grid at all.
	// the set of entities inserted into the grid and the set we call MoveEntity on
	// must match exactly, so both paths ask this same question
	bool IsCollidable(const BoxColliderComponent& col);

	// the single world-space AABB encompassing every hitbox in the collider.
	// insert-time and move-time bounds MUST come from here and nowhere else:
	// if the two disagree the grid's reverse index silently desyncs
	sf::FloatRect GetEntityBounds(const TransformComponent& trans, const BoxColliderComponent& col);

	// called once after loading a level to build the spatial grid
	void BuildSpatialGrid(Registry& registry, math::SpatialGrid& spatialGrid);

	// called when spawning a new entity dynamically after BuildSpatialGrid was already called
	void RegisterEntityCollisions(math::SpatialGrid& grid, const Entity& entity);

	// called every frame in OnFixedUpdate to apply the physics
	void UpdateKinematics(Registry& registry, math::SpatialGrid& grid, float timeStep);
}
