#pragma once
#include <memory>
#include <filesystem>

namespace sc::ecs { class Registry; }
namespace sc::graphics { class TileSetManager; }
namespace sc::world { class World; }
namespace game::factories { class EntityFactory; }

namespace game::world_loader {
	// Parses the LDtk file and returns a fully constructed Engine World.
	// It also spawns all LDtk entities directly into the provided ECS registry.
	std::shared_ptr<sc::world::World> Load(
		const std::filesystem::path& projectFilePath,
		sc::ecs::Registry& registry,
		factories::EntityFactory& entityFactory,
		sc::graphics::TileSetManager& tileSetManager
	);
}
