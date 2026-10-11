#include <pch.h>
#include <blueprints/BlueprintRegistry.h>

#include <blueprints/PlayerBlueprint.h>
#include <blueprints/DragonBlueprint.h>
#include <blueprints/ChestBlueprint.h>
#include <blueprints/WellBlueprint.h>
#include <factories/EntityFactory.h>

namespace game::blueprints {
	using entities::EntityType;

	void RegisterAll(factories::EntityFactory& factory) {
		factory.Register(EntityType::Player, std::make_unique<PlayerBlueprint>());
		factory.Register(EntityType::Dragon, std::make_unique<DragonBlueprint>());
		factory.Register(EntityType::Chest, std::make_unique<ChestBlueprint>());
		factory.Register(EntityType::TeleportationWell, std::make_unique<WellBlueprint>());
	}
}
