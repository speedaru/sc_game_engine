#pragma once
#include <engine/ecs/Registry.h>
#include <engine/math/SpatialGrid.h>
#include <engine/physics/Collisions.h>
#include <engine/world/Level.h>
#include <engine/math/physics.h>

namespace sc::ecs {
	struct TransformComponent;
	struct BoxColliderComponent;
	class Registry;
}

namespace sc::ecs::physics_system  {
	// returns true if collider has hitboxes
	bool ColliderIsCollidable(const BoxColliderComponent& col);

	// get single AABB box encapsulating all other hitboxes
	sf::FloatRect EntityGetBounds(const TransformComponent& trans, const BoxColliderComponent& col);

	// called when spawning a new entity
	void EntityRegisterCollisions(math::SpatialGrid& grid, const Entity& entity);
	void EntityRegisterCollisions(math::SpatialGrid& grid, EntityHandle handle, Registry& registry);

	void EntityUnregisterCollisions(math::SpatialGrid& grid, EntityHandle handle);

	// called every frame in OnFixedUpdate to apply the physics
	// takes the whole Level because collision are stored in
	// the level's spatial grid (entities) and its collision layer (static tile geometry)
	// scratch is owned by the caller so its buffers are reused between steps
	void UpdateKinematics(Registry& registry, world::Level& level, float timeStep, physics::PhysicsScratch& scratch);

	// function to teleport an entity to a location instantly
	void Teleport(math::SpatialGrid& grid, Registry& registry, EntityHandle ent, sf::Vector2f newPos);
}
