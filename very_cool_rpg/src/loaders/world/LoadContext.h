#pragma once
#include <filesystem>

namespace sc::graphics { class TileSetManager; }
namespace game::factories { class EntityFactory; }

namespace game::world_loader {
	class TileColliderCache;

	// bundles everything a layer builder needs to turn LDtk data into engine/ECS objects
	struct LoadContext {
		factories::EntityFactory& entityFactory;
		sc::graphics::TileSetManager& tileSetManager;
		TileColliderCache& tileColliders;
		std::filesystem::path projectDir;
	};
}
