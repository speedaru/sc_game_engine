#include <pch.h>
#include <engine/ecs/systems/PhysicsSystem.h>
#include <engine/ecs/Scene.h>
#include <engine/ecs/Components.h>

namespace sc::ecs::systems {
	void UpdateKinematics(Scene& scene, float timeStep) {
		auto view = scene.GetRegistry().view<TransformComponent, const VelocityComponent>();

		for (auto [entity, trans, vel] : view.each()) {
			trans.pos.x += vel.velocity.x * timeStep;
			trans.pos.y += vel.velocity.y * timeStep;
		}
	}
}
