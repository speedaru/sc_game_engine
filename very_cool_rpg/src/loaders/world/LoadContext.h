#pragma once
#include <filesystem>

#include <engine/graphics/TileSetManager.h>
#include <engine/world/Level.h>

#include <factories/EntityFactory.h>
#include <loaders/world/TileColliderCache.h>

namespace game::world_loader {
	// bundles everything a layer builder needs to turn LDtk data into engine/ECS objects
	struct LoadContext {
		factories::EntityFactory& entityFactory;
		sc::graphics::TileSetManager& tileSetManager;
		TileColliderCache& tileColliders;
		std::filesystem::path projectDir;
	};
}
