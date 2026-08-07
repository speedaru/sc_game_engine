#include <pch.h>
#include <loaders/world/LevelLoader.h>

#include <engine/ecs/systems/physics_system.h>

namespace world = sc::world;

namespace game::world_loader {
	std::shared_ptr<world::Level> LoadLevel(const ldtk::Level& ldtkLevel, const LayerBuilderRegistry& builders, LoadContext& ctx) {
		world::LevelId levelId{ ldtkLevel.name, ldtkLevel.uid };
		sf::Vector2u levelSize(ldtkLevel.size.x, ldtkLevel.size.y);
		std::shared_ptr<world::Level> engineLevel = std::make_shared<world::Level>(levelId, levelSize);

		// entities spawned while building this level's layers index into this level's grid
		ctx.entityFactory.SetCurrentLevel(engineLevel.get());

		// iterate layers from bottom to top (reverse iterator)
		for (auto it = ldtkLevel.allLayers().rbegin(); it != ldtkLevel.allLayers().rend(); ++it) {
			const auto& ldtkLayer = *it;

			if (!ldtkLayer.isVisible()) continue;

			if (auto layer = builders.Dispatch(ldtkLayer, ctx)) {
				engineLevel->AddLayer(std::move(layer));
			}
		}

		// entities spawned through the factory already indexed themselves, but tile
		// colliders are created directly and never touch it, so the sweep still has work
		// to do. it stays a full rebuild rather than a tile-only pass because
		// InsertEntity ignores anything already present
		sc::ecs::physics_system::BuildSpatialGrid(ctx.registry, engineLevel->GetSpatialGrid());

		return engineLevel;
	}
}
