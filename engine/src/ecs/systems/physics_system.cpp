#include <pch.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/Entity.h>
#include <engine/math/physics.h>

using namespace sc::ecs;
namespace math = sc::math;

namespace {
	// whether an entity belongs in the spatial grid at all.
	// the set of entities inserted into the grid and the set we call MoveEntity on
	// must match exactly, so both paths ask this same question
	bool IsCollidable(const BoxColliderComponent& col) {
		return !col.hitboxes.empty();
	}

	// the single world-space AABB encompassing every hitbox in the collider.
	// insert-time and move-time bounds MUST come from here and nowhere else:
	// if the two disagree the grid's reverse index silently desyncs
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

	// narrow phase: sweeps a moving entity against a list of nearby entities.
	// these are not just statics - any collidable entity in the grid blocks movement,
	// including other moving entities
	math::SweepResult FindClosestCollision(
		entt::entity self,
		const TransformComponent& dynTrans,
		const BoxColliderComponent& dynCol,
		const sf::Vector2f& delta,
		const std::vector<entt::entity>& nearbyEntities, // filtered by spatial grid
		entt::registry& reg
	)
	{
		math::SweepResult closestSweep;

		// Check every hitbox in the moving entity
		for (const auto& dynBox : dynCol.hitboxes) {
			sf::FloatRect movingRect(
				{
					dynTrans.pos.x + dynBox.offset.x,
					dynTrans.pos.y + dynBox.offset.y
				},
					{
						dynBox.size.x,
						dynBox.size.y
					}
			);

			// Only sweep against the highly filtered list of nearby entities
			for (entt::entity otherEnt : nearbyEntities) {
				if (otherEnt == self) continue;

				// Safely skip if the other entity was destroyed or lacks components
				if (!reg.all_of<TransformComponent, BoxColliderComponent>(otherEnt)) continue;

				const auto& otherTrans = reg.get<TransformComponent>(otherEnt);
				const auto& otherCol = reg.get<BoxColliderComponent>(otherEnt);

				for (const auto& statBox : otherCol.hitboxes) {
					sf::FloatRect staticRect(
						{
							otherTrans.pos.x + statBox.offset.x,
							otherTrans.pos.y + statBox.offset.y
						},
							{
								statBox.size.x,
								statBox.size.y
							}
					);

					// Perform the pure math check
					auto sweep = math::SweptAABB(movingRect, delta, staticRect);

					if (sweep.time < closestSweep.time) {
						closestSweep = sweep;
					}
				}
			}
		}

		return closestSweep;
	}

	// pipeline orchestrator: runs the broad phase, calls the narrow phase, and handles the slide resolution
	void MoveAndResolve(
		entt::entity self,
		TransformComponent& trans,
		VelocityComponent& vel,
		const BoxColliderComponent& col,
		float timeStep,
		math::SpatialGrid& grid,
		entt::registry& reg
	)
	{
		sf::Vector2f delta = vel.velocity * timeStep;
		const int MAX_SLIDES = 3;

		// If the entity has no hitboxes, just move it and return
		if (col.hitboxes.empty()) {
			trans.pos += delta;
			return;
		}

		for (int i = 0; i < MAX_SLIDES; ++i) {
			if (delta.x == 0.0f && delta.y == 0.0f) break;

			// 1. BROAD PHASE BOUNDING BOX
			// Get the true bounding box that encapsulates all hitboxes
			sf::FloatRect compoundBounds = GetEntityBounds(trans, col);
			sf::FloatRect queryBounds(
				{
					compoundBounds.position.x + std::min(0.0f, delta.x),
					compoundBounds.position.y + std::min(0.0f, delta.y)
				},
				{
					compoundBounds.size.x + std::abs(delta.x),
					compoundBounds.size.y + std::abs(delta.y)
				}
			);

			// 2. BROAD PHASE QUERY
			// Ask the grid *only* for the cells we are moving towards
			std::vector<entt::entity> nearbyEntities = grid.Query(queryBounds);

			// 3. NARROW PHASE
			auto sweep = FindClosestCollision(self, trans, col, delta, nearbyEntities, reg);

			// If the path is entirely clear, move the full distance and exit the loop
			if (sweep.time >= 1.0f) {
				trans.pos += delta;
				break;
			}

			// 4. RESOLUTION (Sliding)
			const float EPSILON = 0.001f;
			float safeTime = std::max(0.0f, sweep.time - EPSILON);

			// Move the entity up to the point of impact
			trans.pos += delta * safeTime;

			// Calculate the remaining time in this frame
			float remainingTime = 1.0f - sweep.time;
			delta = delta * remainingTime;

			// Slide Response: Strip out the velocity going into the wall
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
	void BuildSpatialGrid(Registry& registry, math::SpatialGrid& spatialGrid) {
		auto view = registry.GetRegistry().view<const TransformComponent, const BoxColliderComponent>();

		// for each collider insert it into the grid
		for (auto [entity, trans, collider] : view.each()) {
			// skip entities with no hitboxes: they have no meaningful bounds, and
			// inserting them here would make UpdateKinematics' MoveEntity calls
			// disagree about who is in the grid
			if (!IsCollidable(collider)) continue;

			spatialGrid.InsertEntity(entity, GetEntityBounds(trans, collider));
		}
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

	void UpdateKinematics(Registry& registry, math::SpatialGrid& grid, float timeStep) {
		auto& reg = registry.GetRegistry();

		// Grab all entities that can move and collide
		auto dynamicView = reg.view<TransformComponent, VelocityComponent, const BoxColliderComponent>();

		for (auto [entity, trans, vel, col] : dynamicView.each()) {
			const sf::Vector2f oldPos = trans.pos;

			MoveAndResolve(entity, trans, vel, col, timeStep, grid, reg);

			// Keep the grid in sync with where the entity actually ended up.
			// Done here rather than inside MoveAndResolve's slide loop: the intermediate
			// positions are invisible to everyone (that loop only queries on behalf of
			// this entity, which excludes itself), so one update per step is enough.
			//
			// Updating immediately instead of after the whole view makes resolution
			// sequential - entities later in the view sweep against the final positions
			// of earlier ones, so two entities can never move into the same space. The
			// tradeoff is order dependence: whoever is iterated first wins contested ground.
			if (trans.pos != oldPos && IsCollidable(col)) {
				grid.MoveEntity(entity, GetEntityBounds(trans, col));
			}
		}
	}
}
