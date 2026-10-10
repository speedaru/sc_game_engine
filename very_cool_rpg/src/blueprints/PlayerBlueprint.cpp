#include <pch.h>
#include <blueprints/PlayerBlueprint.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void PlayerBlueprint::Build(sc::ecs::Entity& entity, const entities::SpawnParams& params) const {
		entity.AddComponent<ecs::VelocityComponent>();
		entity.AddComponent<components::PlayerTag>();
		entity.AddComponent<components::CharacterController>();
	}
}
