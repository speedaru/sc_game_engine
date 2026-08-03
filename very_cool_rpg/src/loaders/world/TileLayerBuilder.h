#pragma once
#include <loaders/world/ILayerBuilder.h>

namespace game::world_loader {
	// standard scrolling tile layer (background, ground, decoration, ...)
	// this is the fallback tile-layer builder: register it after any more specific
	// tile-layer builder (e.g. YSortedTileLayerBuilder) so those get first refusal
	class TileLayerBuilder : public ILayerBuilder {
	public:
		bool CanHandle(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>& tags) const override;
		std::unique_ptr<sc::world::ILevelLayer> Build(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>& tags, LoadContext& ctx) const override;
	};

	// tile layer whose sprites depth-sort against entities instead of always rendering below/above them
	// selected via the "YS" tag in the layer name, e.g. "Ground_YS_"
	class YSortedTileLayerBuilder : public ILayerBuilder {
	public:
		static constexpr const char* TAG = "YS";

		bool CanHandle(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>& tags) const override;
		std::unique_ptr<sc::world::ILevelLayer> Build(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>& tags, LoadContext& ctx) const override;
	};
}
