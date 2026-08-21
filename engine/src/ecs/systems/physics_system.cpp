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

	// runs both broad phases, calls the narrow phase, and handles the slide resolution
	void MoveAndResolve(
		entt::entity self,
		TransformComponent& trans,
		VelocityComponent& vel,
		const BoxColliderComponent& col,
		float timeStep,
		sc::world::Level& level,
		entt::registry& reg,
		physics::CandidateBuffer& candidates,
		std::vector<entt::entity>& scratch
	)
	{
		sf::Vector2f delta = vel.velocity * timeStep;
		const int MAX_SLIDES = 3;

		// if entity has no hitboxes, just move it and return
		if (!IsCollidable(col)) {
			trans.pos += delta;
			return;
		}

		if (delta.x == 0.0f && delta.y == 0.0f) return;

		// broad phase, once for the whole step, not once per slide.
		// sliding only ever shortens the remaining delta
		const sf::FloatRect queryBounds = SweptBounds(GetEntityBounds(trans, col), delta);

		// query candidates in collision layer (static tiles), and spatial grid (dynamic entities)
		candidates.clear();
		level.GetCollisionLayer().QueryArea(queryBounds, candidates);
		CollectEntityCandidates(level.GetSpatialGrid(), reg, queryBounds, self, candidates, scratch);

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
	bool IsCollidable(const BoxColliderComponent& col) {
		return !col.hitboxes.empty();
	}

	sf::FloatRect GetEntityBounds(const TransformComponent& trans, const BoxColliderComponent& col) {
		assert(IsCollidable(col) && "GetEntityBounds requires at least one hitbox");

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

	void RegisterEntityCollisions(math::SpatialGrid& grid, const Entity& entity) {
		if (!entity.HasComponent<TransformComponent>() || !entity.HasComponent<BoxColliderComponent>()) {
			LOG_W("trying to register entity %u but it doesnt have transform and box collider component", static_cast<uint32_t>(entity.GetHandle()));
			return;
		}

		const auto& transform = entity.GetComponent<const TransformComponent>();
		const auto& collider = entity.GetComponent<const BoxColliderComponent>();

		// ensure collider has hitboxes
		if (!IsCollidable(collider)) {
			return;
		}

		grid.InsertEntity(entity.GetHandle(), GetEntityBounds(transform, collider));
	}

	void CollectEntityCandidates(
		const math::SpatialGrid& grid,
		entt::registry& reg,
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

			// entities are not yet erased in spatial grid
			if (!reg.valid(other)) continue;

			const auto* otherTrans = reg.try_get<TransformComponent>(other);
			const auto* otherCol = reg.try_get<BoxColliderComponent>(other);
			if (!otherTrans || !otherCol) continue;

			for (const math::Hitbox& hitbox : otherCol->hitboxes) {
				out.push_back(physics::ColliderRef{
					.box = sf::FloatRect{ otherTrans->pos + hitbox.offset, hitbox.size },
					.entity = other
				});
			}
		}
	}

	void UpdateKinematics(Registry& registry, world::Level& level, float timeStep) {
		auto& reg = registry.GetRegistry();

		// grab all entities that can move and collide
		auto dynamicView = reg.view<TransformComponent, VelocityComponent, const BoxColliderComponent>();

		physics::CandidateBuffer candidates;
		std::vector<entt::entity> scratch;

		for (auto [entity, trans, vel, col] : dynamicView.each()) {
			const sf::Vector2f oldPos = trans.pos;

			// move entities and handle collisions
			MoveAndResolve(entity, trans, vel, col, timeStep, level, reg, candidates, scratch);

			// move entities in spatial grid
			if (trans.pos != oldPos && IsCollidable(col)) {
				level.GetSpatialGrid().MoveEntity(entity, GetEntityBounds(trans, col));
			}
		}
	}
}
