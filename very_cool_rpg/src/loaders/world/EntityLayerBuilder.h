#pragma once
#include <loaders/world/ILayerBuilder.h>

namespace game::world_loader {
	// spawns LDtk entities into the ECS registry via the EntityFactory, and applies any
	// custom data authored on the entity's displayed icon tile (e.g. hitboxes)
	struct EntityLayerBuilder : ILayerBuilder {
		bool CanHandle(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>& tags) const override;
		std::unique_ptr<sc::world::ILevelLayer> Build(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>& tags, LoadContext& ctx) const override;
	};
}
