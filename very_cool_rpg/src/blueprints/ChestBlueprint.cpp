#include <pch.h>
#include <blueprints/ChestBlueprint.h>

#include <engine/ecs/Entity.h>

#include <components/GameComponents.h>

namespace game::blueprints {
	void ChestBlueprint::Build(sc::ecs::Entity& entity, const entities::SpawnParams& params) const {
		entity.AddComponent<components::PickupTag>();
	}
}
