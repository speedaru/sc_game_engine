#pragma once
#include <memory>

#include <LDtkLoader/Level.hpp>

#include <engine/world/Level.h>

#include <loaders/world/LoadContext.h>
#include <loaders/world/LayerBuilderRegistry.h>

namespace game::world_loader {
	// builds one engine Level from an LDtk level: walks its layers bottom-to-top,
	// skipping invisible ones, and delegates each to the layer builder registry
	std::shared_ptr<sc::world::Level> LoadLevel(const ldtk::Level& ldtkLevel, const LayerBuilderRegistry& builders, LoadContext& ctx);
}
