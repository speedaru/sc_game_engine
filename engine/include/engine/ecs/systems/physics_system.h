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
	// whether an entity belongs in the spatial grid at all.
	// the set of entities inserted into the grid and the set we call MoveEntity on
	// must match exactly, so both paths ask this same question
	bool IsCollidable(const BoxColliderComponent& col);

	// the single world-space AABB encompassing every hitbox in the collider.
	// insert-time and move-time bounds MUST come from here and nowhere else:
	// if the two disagree the grid's reverse index silently desyncs
	sf::FloatRect GetEntityBounds(const TransformComponent& trans, const BoxColliderComponent& col);

	// called once after loading a level to build the spatial grid, only entities live here
	void BuildSpatialGrid(Registry& registry, math::SpatialGrid& spatialGrid);

	// called when spawning a new entity dynamically after BuildSpatialGrid was already called
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
