#include <pch.h>
#include <blueprints/BlueprintRegistry.h>

#include <blueprints/PlayerBlueprint.h>
#include <blueprints/DragonBlueprint.h>

namespace game::blueprints {
	using entities::EntityType;

	void RegisterAll(factories::EntityFactory& factory) {
		factory.Register(EntityType::Player, std::make_unique<PlayerBlueprint>());
		factory.Register(EntityType::Dragon, std::make_unique<DragonBlueprint>());
	}
}
