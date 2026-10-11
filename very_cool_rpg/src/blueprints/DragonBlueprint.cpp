#include <pch.h>
#include <blueprints/DragonBlueprint.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void DragonBlueprint::Build(sc::ecs::Entity& entity, const entities::SpawnParams& params) const {
		entity.AddComponent<ecs::VelocityComponent>();
	}
}
