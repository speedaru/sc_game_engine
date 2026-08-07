#pragma once
#include <filesystem>

#include <engine/ecs/Registry.h>
#include <engine/graphics/TileSetManager.h>
#include <engine/world/Level.h>

#include <factories/EntityFactory.h>
#include <loaders/world/TileColliderCache.h>

namespace fs = std::filesystem;

namespace game::world_loader {
	// bundles everything a layer builder needs to turn LDtk data into engine/ECS objects
	struct LoadContext {
		sc::ecs::Registry& registry;
		factories::EntityFactory& entityFactory;
		sc::graphics::TileSetManager& tileSetManager;
		TileColliderCache& tileColliders;
		fs::path projectDir;
	};
}
