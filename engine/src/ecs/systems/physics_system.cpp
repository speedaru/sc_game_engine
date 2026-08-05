#include <pch.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/Components.h>
#include <engine/math/physics.h>

using namespace sc::ecs;
namespace math = sc::math;

namespace {
	// get a single hitbox that encapsulates all the hitboxes in the collider
	Hitbox GetSingleHitbox(const std::vector<Hitbox> hitboxes) {
		Hitbox singleHitbox = hitboxes[0]; // use first one to start
		for (const auto& hb : hitboxes) {
			// take smallest for top left
			singleHitbox.offset.x = std::min(singleHitbox.offset.x, hb.offset.x);
			singleHitbox.offset.y = std::min(singleHitbox.offset.y, hb.offset.y);

			// take biggest for size
			singleHitbox.size.x = std::max(singleHitbox.size.x, hb.size.x);
			singleHitbox.size.y = std::max(singleHitbox.size.y, hb.size.y);
		}

		return singleHitbox;
	}

	sf::FloatRect HitboxToLevelCoords(const sf::Vector2f& pos, const Hitbox& hitbox) {
		return sf::FloatRect{
			pos + hitbox.offset,
			 hitbox.size
		};
	}

	// narrow phase: sweeps a moving entity against a list of static entities
	math::SweepResult FindClosestCollision(
		entt::entity self,
		const TransformComponent& dynTrans,
		const BoxColliderComponent& dynCol,
		const sf::Vector2f& delta,
		const std::vector<entt::entity>& nearbyStatics, // filtered by spatial grid
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

			// Only sweep against the highly filtered list of nearby statics
			for (entt::entity staticEnt : nearbyStatics) {
				if (staticEnt == self) continue;

				// Safely skip if the static entity was destroyed or lacks components
				if (!reg.all_of<TransformComponent, BoxColliderComponent>(staticEnt)) continue;

				const auto& staticTrans = reg.get<TransformComponent>(staticEnt);
				const auto& staticCol = reg.get<BoxColliderComponent>(staticEnt);

				for (const auto& statBox : staticCol.hitboxes) {
					sf::FloatRect staticRect(
						{
							staticTrans.pos.x + statBox.offset.x,
							staticTrans.pos.y + statBox.offset.y
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
			// We use the first hitbox to calculate the movement bounds.
			// std::min(0.0f, delta) ensures the box stretches backward if moving left/up.
			// std::abs(delta) ensures the box stretches forward if moving right/down.
			const auto& mainBox = col.hitboxes.front();
			sf::FloatRect queryBounds(
				{
					trans.pos.x + mainBox.offset.x + std::min(0.0f, delta.x),
					trans.pos.y + mainBox.offset.y + std::min(0.0f, delta.y)
				},
					{
						mainBox.size.x + std::abs(delta.x),
						mainBox.size.y + std::abs(delta.y)
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
			// to convert hitbox coords to level pos
			Hitbox singleHitbox = GetSingleHitbox(collider.hitboxes);

			spatialGrid.InsertEntity(entity, HitboxToLevelCoords(trans.pos, singleHitbox));
		}
	}

	void UpdateKinematics(Registry& registry, math::SpatialGrid& grid, float timeStep) {
		auto& reg = registry.GetRegistry();

		// Grab all entities that can move and collide
		auto dynamicView = reg.view<TagComponent, TransformComponent, VelocityComponent, const BoxColliderComponent>();

		for (auto [entity, tag, trans, vel, col] : dynamicView.each()) {
			MoveAndResolve(entity, trans, vel, col, timeStep, grid, reg);
		}
	}
}
