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
	bool IsCollidable(const BoxColliderComponent& col);

	// get single AABB box encapsulating all other hitboxes
	sf::FloatRect GetEntityBounds(const TransformComponent& trans, const BoxColliderComponent& col);

	// called when spawning a new entity
	void RegisterEntityCollisions(math::SpatialGrid& grid, const Entity& entity);

	// resolves every entity overlapping area into world space boxes and appends them to
	// out, skipping self. the entity half of the broad phase; CollisionLayer::QueryArea
	// is the other half, and both append to the same buffer
	void CollectEntityCandidates(
		const math::SpatialGrid& grid,
		entt::registry& reg,
		const sf::FloatRect& area,
		entt::entity self,
		physics::CandidateBuffer& out,
		std::vector<entt::entity>& scratch
	);

	// called every frame in OnFixedUpdate to apply the physics.
	// takes the whole Level because collision has two sources: the level's spatial grid
	// (entities) and its collision layer (static tile geometry)
	void UpdateKinematics(Registry& registry, world::Level& level, float timeStep);
}
