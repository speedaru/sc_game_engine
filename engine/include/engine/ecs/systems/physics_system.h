#pragma once
#include <engine/ecs/Registry.h>
#include <engine/math/SpatialGrid.h>
#include <engine/physics/ColliderRef.h>
#include <engine/world/Level.h>

namespace sc::ecs {
	struct TransformComponent;
	struct BoxColliderComponent;
}

namespace sc::ecs::physics_system  {
	// returns true if collider has hitboxes
	bool ColliderIsCollidable(const BoxColliderComponent& col);

	// get single AABB box encapsulating all other hitboxes
	sf::FloatRect EntityGetBounds(const TransformComponent& trans, const BoxColliderComponent& col);

	// called when spawning a new entity
	void EntityRegisterCollisions(math::SpatialGrid& grid, const Entity& entity);

	// called every frame in OnFixedUpdate to apply the physics
	// takes the whole Level because collision are stored in
	// the level's spatial grid (entities) and its collision layer (static tile geometry)
	void UpdateKinematics(Registry& registry, world::Level& level, float timeStep);

	// function to teleport an entity to a location instantly
	void Teleport(math::SpatialGrid& grid, Entity& entity, sf::Vector2f newPos);
}
