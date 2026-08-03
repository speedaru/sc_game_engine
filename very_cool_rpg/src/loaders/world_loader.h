#pragma once
#include <memory>
#include <filesystem>

#include <engine/world/World.h>
#include <engine/ecs/Registry.h>
#include <engine/graphics/TileSetManager.h>

#include <factories/EntityFactory.h>

namespace fs = std::filesystem;

namespace game::world_loader {
	// Parses the LDtk file and returns a fully constructed Engine World.
	// It also spawns all LDtk entities directly into the provided ECS registry.
	std::shared_ptr<sc::world::World> Load(
		const fs::path& projectFilePath,
		sc::ecs::Registry& registry,
		factories::EntityFactory& entityFactory,
		sc::graphics::TileSetManager& tileSetManager
	);
}
