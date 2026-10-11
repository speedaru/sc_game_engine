#pragma once
#include <memory>

#include <loaders/world/LayerBuilderRegistry.h>

namespace ldtk { class Level; }
namespace sc::world { class Level; }

namespace game::world_loader {
	struct LoadContext;

	// builds one engine Level from an LDtk level: walks its layers bottom-to-top,
	// skipping invisible ones, and delegates each to the layer builder registry
	std::shared_ptr<sc::world::Level> LoadLevel(const ldtk::Level& ldtkLevel, const LayerBuilderRegistry& builders, LoadContext& ctx);
}
