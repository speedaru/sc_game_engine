#include <pch.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/Components.h>
#include <engine/math/physics.h>

using namespace sc::ecs;
namespace math = sc::math;

namespace {
	// We use C++20 'auto&' to easily pass the complex EnTT view type without massive templates
	math::SweepResult FindClosestCollision(
		const TransformComponent& dynTrans,
		const BoxColliderComponent& dynCol,
		const sf::Vector2f& delta,
		auto& staticView
	) 
	{
		math::SweepResult closestSweep;

		// Check every hitbox in the dynamic entity (Compound Colliders)
		for (const auto& dynBox : dynCol.hitboxes) {
			sf::FloatRect movingRect(
				{
					dynTrans.pos.x + dynBox.offset.x,
					dynTrans.pos.y + dynBox.offset.y,
				},
				{
					dynBox.size.x,
					dynBox.size.y
				}
			);

			// Sweep against every static entity in the world
			for (auto [staticEnt, staticTrans, staticCol] : staticView.each()) {
				for (const auto& statBox : staticCol.hitboxes) {
					sf::FloatRect staticRect(
						{
							staticTrans.pos.x + statBox.offset.x,
							staticTrans.pos.y + statBox.offset.y,
						},
						{
							statBox.size.x,
							statBox.size.y
						}
					);

					auto sweep = math::SweptAABB(movingRect, delta, staticRect);

					if (sweep.time < closestSweep.time) {
						closestSweep = sweep;
					}
				}
			}
		}

		return closestSweep;
	}

	void MoveAndResolve(
		TransformComponent& trans,
		VelocityComponent& vel,
		const BoxColliderComponent& col,
		float timeStep,
		auto& staticView
	)
	{
		sf::Vector2f delta = vel.velocity * timeStep;
		const int MAX_SLIDES = 3;

		// Only log if the entity is actually trying to move this frame
		bool isMoving = (delta.x != 0.0f || delta.y != 0.0f);

		for (int i = 0; i < MAX_SLIDES; ++i) {
			if (delta.x == 0.0f && delta.y == 0.0f) {
				break;
			}

			auto sweep = FindClosestCollision(trans, col, delta, staticView);

			// If time == 1.0f, no collisions happened. Apply full movement and exit.
			if (sweep.time >= 1.0f) {
				trans.pos += delta;
				break;
			}

			const float EPSILON = 0.001f;
			float safeTime = std::max(0.0f, sweep.time - EPSILON);

			// Move entity up to the point of contact
			trans.pos += delta * safeTime;

			// Calculate the remaining time in this frame after impact
			float remainingTime = 1.0f - sweep.time;
			delta = delta * remainingTime;

			// Slide Response
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
	void UpdateKinematics(Registry& registry, float timeStep) {
		auto& reg = registry.GetRegistry();
        
        // 1. The Environment: Grab everything that has a collider but DOES NOT move
        auto staticView = reg.view<const TransformComponent, const BoxColliderComponent>(entt::exclude<VelocityComponent>);

        // 2. The Actors: Grab everything that has a collider AND moves
        auto dynamicView = reg.view<TransformComponent, VelocityComponent, const BoxColliderComponent>();
        
        for (auto [entity, trans, vel, col] : dynamicView.each()) {
            MoveAndResolve(trans, vel, col, timeStep, staticView);
        }
	}
}
