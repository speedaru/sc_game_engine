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
		try {
			ldtkProject.loadFromFile(projectFilePath.string());
		}
		catch (const std::exception& e) {
			LOG_E("failed to load ldtk project '%s': %s", projectFilePath.string().c_str(), e.what());
			return nullptr;
		}

		const fs::path projectDir = projectFilePath.parent_path();

		entityFactory.SetRegistry(registry);

		LoadEntityDefinitions(ldtkProject, entityFactory, projectDir);

		TileColliderCache tileColliders;
		LoadContext ctx{ registry, entityFactory, tileSetManager, tileColliders, projectDir };
		LayerBuilderRegistry builders = MakeDefaultLayerBuilderRegistry();

		// every ldtk world goes into the one engine world. level uids are unique across the
		// whole project, so a teleport between ldtk worlds is a normal level switch
		auto world = std::make_shared<world::World>();
		try {
			for (const auto& ldtkWorld : ldtkProject.allWorlds()) {
				for (const auto& ldtkLevel : ldtkWorld.allLevels()) {
					world->AddLevel(LoadLevel(ldtkLevel, builders, ctx));
				}
			}
		}
		catch (const std::exception& e) {
			LOG_E("failed to build levels from ldtk project '%s': %s", projectFilePath.string().c_str(), e.what());
			entityFactory.SetCurrentLevel(nullptr);
			return nullptr;
		}

		// unset current level
		entityFactory.SetCurrentLevel(nullptr);

		return world;
	}
}
