#pragma once
#include <memory>
#include <string>
#include <unordered_set>

#include <engine/world/ILevelLayer.h>

namespace ldtk { class Layer; }

namespace game::world_loader {
	struct LoadContext;

	// a strategy for turning one LDtk layer into one engine ILevelLayer.
	// register new layer kinds (new tags, new layer behaviors) by adding a new
	// implementation and registering it - LevelLoader never needs to change.
	class ILayerBuilder {
	public:
		virtual ~ILayerBuilder() = default;

		virtual bool CanHandle(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>& tags) const = 0;
		virtual std::unique_ptr<sc::world::ILevelLayer> Build(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>& tags, LoadContext& ctx) const = 0;
	};
}
