#include <pch.h>
#include <blueprints/BlueprintRegistry.h>

#include <blueprints/PlayerBlueprint.h>
#include <blueprints/DragonBlueprint.h>

namespace game::blueprints {
	void RegisterAll(factories::EntityFactory& factory, const fs::path& projDir) {
		factory.Register("Player", std::make_unique<PlayerBlueprint>(projDir, factory.GetTextureCache()));
		factory.Register("Dragon", std::make_unique<DragonBlueprint>(projDir, factory.GetTextureCache()));
	}
}
