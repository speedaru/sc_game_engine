#include <pch.h>
#include <blueprints/WellBlueprint.h>

#include <engine/ecs/Entity.h>
#include <engine/utils/logging.h>

#include <components/GameComponents.h>
#include <entities/EntityField.h>
#include <entities/SpawnParams.h>

namespace game::blueprints {
	void WellBlueprint::Build(sc::ecs::Entity& entity, const entities::SpawnParams& params) const {
		const entities::EntityRefValue* destination = params.fields.TryGet<entities::EntityRefValue>("destination");
		if (!destination) {
			LOG_E("no destination field");
			return;
		}
		
		entity.AddComponent<components::TeleportComponent>(destination->levelUid, destination->pos);
	}
}
