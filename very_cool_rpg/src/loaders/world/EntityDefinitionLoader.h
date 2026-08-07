#pragma once
#include <filesystem>

#include <LDtkLoader/Project.hpp>

namespace fs = std::filesystem;

namespace game::factories { class EntityFactory; }

namespace game::world_loader {
	// resolves every registered entity type's ldtk::EntityDef into an EntityDefinition,
	// once, before any level is loaded
	//
	// this file and EntityLayerBuilder are the only two that see both LDtk types and
	// entity spawning types. everything else can spawn entities with no
	// ldtk data involved
	void LoadEntityDefinitions(const ldtk::Project& project, factories::EntityFactory& factory, const fs::path& projectDir);
}
