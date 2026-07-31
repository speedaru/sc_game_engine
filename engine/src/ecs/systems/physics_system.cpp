#include <pch.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/Components.h>

namespace sc::ecs::physics_system {
	void UpdateKinematics(Registry& scene, float timeStep) {
		auto view = scene.GetRegistry().view<TransformComponent, const VelocityComponent>();

		for (auto [entity, trans, vel] : view.each()) {
			trans.pos.x += vel.velocity.x * timeStep;
			trans.pos.y += vel.velocity.y * timeStep;
		}
	}
}
