#include <pch.h>
#include <loaders/world/LayerBuilderRegistry.h>

#include <engine/utils/logging.h>

#include <loaders/world/LayerTags.h>
#include <loaders/world/TileLayerBuilder.h>
#include <loaders/world/EntityLayerBuilder.h>

namespace world = sc::world;

namespace game::world_loader {
	void LayerBuilderRegistry::Register(std::unique_ptr<ILayerBuilder> builder) {
		m_builders.push_back(std::move(builder));
	}

	std::unique_ptr<world::ILevelLayer> LayerBuilderRegistry::Dispatch(const ldtk::Layer& ldtkLayer, LoadContext& ctx) const {
		auto tags = ExtractLayerTags(ldtkLayer.getName());

		for (const auto& builder : m_builders) {
			if (builder->CanHandle(ldtkLayer, tags)) {
				return builder->Build(ldtkLayer, tags, ctx);
			}
		}

		LOG_W("No layer builder registered for layer: %s", ldtkLayer.getName().c_str());
		return nullptr;
	}

	LayerBuilderRegistry MakeDefaultLayerBuilderRegistry() {
		LayerBuilderRegistry registry;
		registry.Register(std::make_unique<YSortedTileLayerBuilder>());
		registry.Register(std::make_unique<TileLayerBuilder>());
		registry.Register(std::make_unique<EntityLayerBuilder>());
		return registry;
	}
}
