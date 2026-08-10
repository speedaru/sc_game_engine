#include <pch.h>
#include <loaders/world/LevelLoader.h>

#include <loaders/world/collision_baker.h>

namespace world = sc::world;

namespace game::world_loader {
	std::shared_ptr<world::Level> LoadLevel(const ldtk::Level& ldtkLevel, const LayerBuilderRegistry& builders, LoadContext& ctx) {
		world::LevelId levelId{ ldtkLevel.name, ldtkLevel.uid };
		sf::Vector2u levelSize(ldtkLevel.size.x, ldtkLevel.size.y);
		std::shared_ptr<world::Level> engineLevel = std::make_shared<world::Level>(levelId, levelSize);

		// entities spawned while building this level's layers index into this level's grid
		ctx.entityFactory.SetCurrentLevel(engineLevel.get());

		// static collision first, in its own pass.
		//
		// baking is deliberately not a layer builder: it produces no ILevelLayer, and
		// several source layers at different resolutions collapse into this single grid, so
		// it can't be expressed as one-layer-in-one-layer-out
		engineLevel->SetCollisionLayer(BakeLevelCollision(ldtkLevel, ctx.tileColliders));

		// iterate layers from bottom to top (reverse iterator)
		for (auto it = ldtkLevel.allLayers().rbegin(); it != ldtkLevel.allLayers().rend(); ++it) {
			const auto& ldtkLayer = *it;

			if (!ldtkLayer.isVisible()) continue;

			if (auto layer = builders.Dispatch(ldtkLayer, ctx)) {
				engineLevel->AddLayer(std::move(layer));
			}
		}

		// no BuildSpatialGrid sweep here any more. it existed because tile colliders were
		// created directly and never touched the factory; now that static geometry lives in
		// the collision layer, every entity in the level got there through
		// EntityFactory::Spawn, which indexed it on the way

		return engineLevel;
	}
}
