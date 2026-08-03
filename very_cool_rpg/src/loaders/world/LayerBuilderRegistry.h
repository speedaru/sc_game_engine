#pragma once
#include <vector>
#include <memory>

#include <loaders/world/ILayerBuilder.h>

namespace game::world_loader {
	// ordered set of layer builders; the first one whose CanHandle() accepts a layer wins
	class LayerBuilderRegistry {
	public:
		void Register(std::unique_ptr<ILayerBuilder> builder);

		// returns nullptr (and logs a warning) if no registered builder can handle this layer
		std::unique_ptr<sc::world::ILevelLayer> Dispatch(const ldtk::Layer& ldtkLayer, LoadContext& ctx) const;

	private:
		std::vector<std::unique_ptr<ILayerBuilder>> m_builders;
	};

	// registers the engine's default layer builders in priority order:
	// YSortedTileLayerBuilder -> TileLayerBuilder -> EntityLayerBuilder
	// add a new tag/layer kind by registering an additional builder here
	LayerBuilderRegistry MakeDefaultLayerBuilderRegistry();
}
