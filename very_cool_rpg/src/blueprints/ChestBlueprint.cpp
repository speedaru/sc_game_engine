#include <pch.h>
#include <blueprints/ChestBlueprint.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void ChestBlueprint::Build(sc::ecs::Entity& entity, const entities::SpawnParams& params) const {
		entity.AddComponent<components::PickupTag>();
	}
}
