#include <pch.h>

#include <engine/ecs/Scene.h>
#include <engine/ecs/Components.h>

#include <components/GameComponents.h>

namespace ecs = sc::ecs;

namespace game::systems {
	void UpdateCharacterMovement(sc::ecs::Scene& scene, float timeStep) {
		auto view = scene.GetRegistry().view<components::CharacterController, ecs::VelocityComponent>();

		for (auto [entity, controller, vel] : view.each()) {
			// normalize direction to prevent diagonal speed boost
			float length = std::sqrt(controller.direction.x * controller.direction.x + controller.direction.y * controller.direction.y);
			sf::Vector2f normalDirection = controller.direction;
			if (length > 0.0f) {
				normalDirection.x /= length;
				normalDirection.y /= length;
			}

			// apply acceleration and friction
			if (length > 0.0f) {
				vel.velocity.x += normalDirection.x * controller.acceleration * timeStep;
				vel.velocity.y += normalDirection.y * controller.acceleration * timeStep;

				// clamp max speed
				float currentSpeed = std::sqrt(vel.velocity.x * vel.velocity.x + vel.velocity.y * vel.velocity.y);
				if (currentSpeed > controller.maxSpeed) {
					vel.velocity.x = (vel.velocity.x / currentSpeed) * controller.maxSpeed;
					vel.velocity.y = (vel.velocity.y / currentSpeed) * controller.maxSpeed;
				}
			}
			else {
				// apply friction to smooth stop
				float currentSpeed = std::sqrt(vel.velocity.x * vel.velocity.x + vel.velocity.y * vel.velocity.y);
				if (currentSpeed > 0.0f) {
					float drop = controller.friction * timeStep;
					float multiplier = std::max(currentSpeed - drop, 0.0f) / currentSpeed;
					vel.velocity.x *= multiplier;
					vel.velocity.y *= multiplier;
				}
			}
		}
	}
}