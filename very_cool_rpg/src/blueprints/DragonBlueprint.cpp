#include <pch.h>
#include <blueprints/DragonBlueprint.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void DragonBlueprint::Build(sc::ecs::Entity& entity, const entities::SpawnParams& params) const {
		// transform, sprite and collider are already attached by EntityFactory

		// engine components
		entity.AddComponent<ecs::VelocityComponent>();

		// game components
		entity.AddComponent<components::CharacterController>();
	}
}
