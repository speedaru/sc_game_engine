#pragma once
#include <memory>
#include <filesystem>
#include <string>

#include <engine/world/World.h>
#include <engine/ecs/Registry.h>

#include <factories/EntityFactory.h>

namespace fs = std::filesystem;

namespace game::world_loader {
	inline constexpr const char* YSORTED_LAYER_TAG = "_YS_";

	// Parses the LDtk file and returns a fully constructed Engine World.
	// It also spawns all LDtk entities directly into the provided ECS registry.
	std::shared_ptr<sc::world::World> Load(
		const fs::path& projectFilePath,
		sc::ecs::Registry& registry,
		factories::EntityFactory& entityFactory
	);
}
