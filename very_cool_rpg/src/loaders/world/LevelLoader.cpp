#include <pch.h>
#include <loaders/world/LevelLoader.h>

#include <loaders/world/collision_baker.h>

namespace world = sc::world;

namespace game::world_loader {
	std::shared_ptr<world::Level> LoadLevel(const ldtk::Level& ldtkLevel, const LayerBuilderRegistry& builders, LoadContext& ctx) {
		world::LevelId levelId{ ldtkLevel.name, ldtkLevel.uid };
		sf::Vector2u levelSize(ldtkLevel.size.x, ldtkLevel.size.y);
		std::shared_ptr<world::Level> engineLevel = std::make_shared<world::Level>(levelId, levelSize);

		// spawned entities will go in this level
		ctx.entityFactory.SetCurrentLevel(engineLevel.get());

		// static tile collisions
		engineLevel->SetCollisionLayer(BakeLevelCollision(ldtkLevel, ctx.tileColliders));

		// iterate layers from bottom to top
		for (auto it = ldtkLevel.allLayers().rbegin(); it != ldtkLevel.allLayers().rend(); ++it) {
			const auto& ldtkLayer = *it;

			if (!ldtkLayer.isVisible()) continue;

			if (auto layer = builders.Dispatch(ldtkLayer, ctx)) {
				engineLevel->AddLayer(std::move(layer));
			}
		}

		return engineLevel;
	}
}
