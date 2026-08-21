#include <pch.h>
#include <loaders/world_loader.h>

#include <engine/utils/logging.h>

#include <loaders/world/LoadContext.h>
#include <loaders/world/EntityDefinitionLoader.h>
#include <loaders/world/LayerBuilderRegistry.h>
#include <loaders/world/LevelLoader.h>
#include <loaders/world/TileColliderCache.h>

namespace ecs = sc::ecs;
namespace world = sc::world;
namespace gfx = sc::graphics;

namespace game::world_loader {
	std::shared_ptr<world::World> Load(
		const fs::path& projectFilePath,
		ecs::Registry& registry,
		factories::EntityFactory& entityFactory,
		gfx::TileSetManager& tileSetManager
	) {
		if (!fs::exists(projectFilePath)) {
			LOG_E("project file: '%s' doesn't exist", projectFilePath.string().c_str());
			return nullptr;
		}

		ldtk::Project ldtkProject;
		ldtkProject.loadFromFile(projectFilePath.string());

		const fs::path projectDir = projectFilePath.parent_path();

		entityFactory.SetRegistry(registry);

		LoadEntityDefinitions(ldtkProject, entityFactory, projectDir);

		TileColliderCache tileColliders;
		LoadContext ctx{ registry, entityFactory, tileSetManager, tileColliders, projectDir };
		LayerBuilderRegistry builders = MakeDefaultLayerBuilderRegistry();

		auto world = std::make_shared<world::World>();
		for (const auto& ldtkLevel : ldtkProject.getWorld("").allLevels()) {
			world->AddLevel(LoadLevel(ldtkLevel, builders, ctx));
		}

		// unset current level
		entityFactory.SetCurrentLevel(nullptr);

		return world;
	}
}
