#include <pch.h>
#include <engine/ecs/EntityFactory.h>

#include <blueprints/PlayerBlueprint.h>
#include <blueprints/DragonBlueprint.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void RegisterAll(ecs::EntityFactory& factory) {
		factory.Register("Player", std::make_unique<PlayerBlueprint>());
		factory.Register("Dragon", std::make_unique<DragonBlueprint>());
	}
}
