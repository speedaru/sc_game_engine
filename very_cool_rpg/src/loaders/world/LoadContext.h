#pragma once
#include <filesystem>

#include <engine/ecs/Registry.h>
#include <engine/graphics/TileSetManager.h>
#include <engine/world/Level.h>

#include <factories/EntityFactory.h>

namespace fs = std::filesystem;

namespace game::world_loader {
	// bundles everything a layer builder needs to turn LDtk data into engine/ECS objects
	struct LoadContext {
		sc::ecs::Registry& registry;
		factories::EntityFactory& entityFactory;
		sc::graphics::TileSetManager& tileSetManager;
		fs::path projectDir;
		sc::world::Level* currentLevel; // raw ptr bcs ctx lives as long as level
	};
}
