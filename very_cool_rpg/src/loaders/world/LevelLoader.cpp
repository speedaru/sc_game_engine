#include <pch.h>
#include <loaders/world/LevelLoader.h>

namespace world = sc::world;

namespace game::world_loader {
	std::shared_ptr<world::Level> LoadLevel(const ldtk::Level& ldtkLevel, const LayerBuilderRegistry& builders, LoadContext& ctx) {
		world::LevelId levelId{ ldtkLevel.name, ldtkLevel.uid };
		auto engineLevel = std::make_shared<world::Level>(levelId, sf::Vector2i(ldtkLevel.size.x, ldtkLevel.size.y));

		// iterate layers from bottom to top (reverse iterator)
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
