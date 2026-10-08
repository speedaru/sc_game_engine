#include <pch.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/Entity.h>
#include <engine/math/physics.h>

using namespace sc::ecs;
using namespace sc::ecs::physics_system;
namespace math = sc::math;
namespace physics = sc::physics;

constexpr float EPSILON = 0.001f;

namespace {
	// narrow phase: sweeps a moving entity against already resolved boxes.
	math::SweepResult FindClosestCollision(
		const TransformComponent& dynTrans,
		const BoxColliderComponent& dynCol,
		const sf::Vector2f& delta,
		const physics::CandidateBuffer& candidates
	)
	{
		math::SweepResult closestSweep;

		for (const math::Hitbox& dynBox : dynCol.hitboxes) {
			const sf::FloatRect movingRect{ dynTrans.pos + dynBox.offset, dynBox.size };

			for (const physics::ColliderRef& other : candidates) {
				const math::SweepResult sweep = math::SweptAABB(movingRect, delta, other.box);

				if (sweep.time < closestSweep.time) {
					closestSweep = sweep;
				}
			}
		}

		return closestSweep;
	}

	sf::FloatRect SweptBounds(const sf::FloatRect& bounds, const sf::Vector2f& delta) {
		return sf::FloatRect{
			{
				bounds.position.x + std::min(0.0f, delta.x),
				bounds.position.y + std::min(0.0f, delta.y)
			},
			{
				bounds.size.x + std::abs(delta.x),
				bounds.size.y + std::abs(delta.y)
			}
		};
	}

	// resolves every entity overlapping area into world space boxes and appends them to
	// out, skipping self. the entity half of the broad phase; CollisionLayer::QueryArea
	// is the other half, and both append to the same buffer
	void EntityCollectCandidates(
		const math::SpatialGrid& grid,
		Registry& registry,
		const sf::FloatRect& area,
		entt::entity self,
		physics::CandidateBuffer& out,
		std::vector<entt::entity>& scratch
	)
	{
		scratch.clear();
		grid.Query(area, scratch);

		for (entt::entity other : scratch) {
			if (other == self) continue;

			const auto* otherTrans = registry.TryGetComponent<TransformComponent>(other);
			const auto* otherCol = registry.TryGetComponent<BoxColliderComponent>(other);
			if (!otherTrans || !otherCol) continue;

			for (const math::Hitbox& hitbox : otherCol->hitboxes) {
				out.push_back(physics::ColliderRef{
					.box = sf::FloatRect{ otherTrans->pos + hitbox.offset, hitbox.size },
					.entity = other
					});
			}
		}
	}

	// runs both broad phases, calls the narrow phase, and handles the slide resolution
	void MoveAndResolve(
		entt::entity self,
		TransformComponent& trans,
		VelocityComponent& vel,
		const BoxColliderComponent& col,
		float timeStep,
		sc::world::Level& level,
		Registry& registry,
		physics::CandidateBuffer& candidates,
		std::vector<entt::entity>& scratch
	)
	{
		sf::Vector2f delta = vel.velocity * timeStep;
		const int MAX_SLIDES = 3;

		// if entity has no hitboxes, just move it and return
		if (!ColliderIsCollidable(col)) {
			trans.pos += delta;
			return;
		}

		if (delta.x == 0.0f && delta.y == 0.0f) return;

		// broad phase, once for the whole step, not once per slide.
		// sliding only ever shortens the remaining delta
		const sf::FloatRect queryBounds = SweptBounds(EntityGetBounds(trans, col), delta);

		// query candidates in collision layer (static tiles), and spatial grid (dynamic entities)
		candidates.clear();
		level.GetCollisionLayer().QueryArea(queryBounds, candidates);
		EntityCollectCandidates(level.GetSpatialGrid(), registry, queryBounds, self, candidates, scratch);

		for (int i = 0; i < MAX_SLIDES; i++) {
			if (delta.x == 0.0f && delta.y == 0.0f) break;

			// narrow phase
			const math::SweepResult sweep = FindClosestCollision(trans, col, delta, candidates);

			// if the path is entirely clear, move the full distance and exit the loop
			if (sweep.time >= 1.0f) {
				trans.pos += delta;
				break;
			}

			// sliding
			float safeTime = std::max(0.0f, sweep.time - EPSILON);

			// move the entity up to the point of impact
			trans.pos += delta * safeTime;

			// calculate remaining time in this frame
			float remainingTime = 1.0f - sweep.time;
			delta = delta * remainingTime;

			// remove velocity going into wall
			if (sweep.normal.x != 0.0f) {
				delta.x = 0.0f;
				vel.velocity.x = 0.0f;
			}
			if (sweep.normal.y != 0.0f) {
				delta.y = 0.0f;
				vel.velocity.y = 0.0f;
			}
		}
	}
}

namespace sc::ecs::physics_system {
	bool ColliderIsCollidable(const BoxColliderComponent& col) {
		return !col.hitboxes.empty();
	}

	sf::FloatRect EntityGetBounds(const TransformComponent& trans, const BoxColliderComponent& col) {
		assert(ColliderIsCollidable(col) && "GetEntityBounds requires at least one hitbox");

		// use first hitbox as initial values
		sf::Vector2f tl = col.hitboxes[0].offset;
		sf::Vector2f br = tl + col.hitboxes[0].size;

		for (const auto& hb : col.hitboxes) {
			// use most topleft pos for tl
			tl.x = std::min(tl.x, hb.offset.x);
			tl.y = std::min(tl.y, hb.offset.y);

			// use most bottom right pos for br
			br.x = std::max(br.x, hb.offset.x + hb.size.x);
			br.y = std::max(br.y, hb.offset.y + hb.size.y);
		}

		// convert tl + br to level space pos + size
		return sf::FloatRect{ trans.pos + tl, br - tl };
	}

	void EntityRegisterCollisions(math::SpatialGrid& grid, const Entity& entity) {
		if (!entity.HasComponent<TransformComponent>() || !entity.HasComponent<BoxColliderComponent>()) {
			LOG_W("trying to register entity %u but it doesnt have transform and box collider component", static_cast<uint32_t>(entity.GetHandle()));
			return;
		}

		const auto& transform = entity.GetComponent<const TransformComponent>();
		const auto& collider = entity.GetComponent<const BoxColliderComponent>();

		// ensure collider has hitboxes
		if (!ColliderIsCollidable(collider)) {
			return;
		}

		grid.InsertEntity(entity.GetHandle(), EntityGetBounds(transform, collider));
	}

	void EntityRegisterCollisions(math::SpatialGrid& grid, EntityHandle handle, Registry& registry) {
		EntityRegisterCollisions(grid, Entity(handle, &registry));
	}

	void EntityUnregisterCollisions(math::SpatialGrid& grid, EntityHandle handle) {
		// non collidable entities are never inserted so only erase tracked ones
		if (grid.FindEntityCells(handle)) {
			grid.EraseEntity(handle);
		}
	}

	void UpdateKinematics(Registry& registry, world::Level& level, float timeStep) {
		// grab all entities that can move and collide
		auto dynamicEntitiesView = registry.ViewLevel<
			TransformComponent,
			VelocityComponent,
			const BoxColliderComponent
		>(level.GetUid());

		physics::CandidateBuffer candidates;
		std::vector<entt::entity> scratch;

		for (auto [entity, trans, vel, col] : dynamicEntitiesView) {
			const sf::Vector2f oldPos = trans.pos;
			trans.prevPos = oldPos;

			// move entities and handle collisions
			MoveAndResolve(entity, trans, vel, col, timeStep, level, registry, candidates, scratch);

			// move entities in spatial grid
			if (trans.pos != oldPos && ColliderIsCollidable(col)) {
				level.GetSpatialGrid().MoveEntity(entity, EntityGetBounds(trans, col));
			}
		}
	}

	void Teleport(math::SpatialGrid& grid, Entity& entity, sf::Vector2f newPos) {
		auto& trans = entity.GetComponent<TransformComponent>();

		// update entity position
		trans.prevPos = newPos;
		trans.pos = newPos;

		// update grid if collidable
		if (!entity.HasComponent<BoxColliderComponent>()) return;

		auto& collider = entity.GetComponent<BoxColliderComponent>();
		if (ColliderIsCollidable(collider)) {
			// get new entity bounds
			sf::FloatRect newBounds = EntityGetBounds(trans, collider);

			// update level spatial grid
			grid.MoveEntity(entity.GetHandle(), newBounds);
		}
	}
}
