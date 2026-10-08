#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <engine/world/Level.h>

#include <blueprints/IEntityBlueprint.h>
#include <entities/EntityDefinition.h>
#include <entities/EntityType.h>
#include <entities/SpawnParams.h>

namespace sc::ecs {
	class Entity;
	class Registry;
}

namespace game::factories {
	// owns the entity type -> {definition, blueprint} mapping and is the single place
	// an entity is allowed to come into existence. nothing outside Spawn may add components 
	// that the spatial grid cares about, because Spawn indexes the entity the moment it is
	// finished building it
	class EntityFactory {
	public:
		using EntityType = entities::EntityType;

		void Register(EntityType type, std::unique_ptr<blueprints::IEntityBlueprint> blueprint);

		// filled in by EntityDefinitionLoader before any level loads
		void SetDefinition(EntityType type, entities::EntityDefinition definition);

		void SetRegistry(sc::ecs::Registry& registry) { m_registry = &registry; }

		// set the level in which new entities will spawn into
		void SetCurrentLevel(sc::world::Level* level) { m_currentLevel = level; }

		// spawns a new entity in m_currentLevel
		// returns a null Entity if the type has no blueprint or no definition
		sc::ecs::Entity Spawn(EntityType type, const entities::SpawnParams& params);

	private:
		std::unordered_map<EntityType, std::unique_ptr<blueprints::IEntityBlueprint>> m_blueprints;
		std::unordered_map<EntityType, entities::EntityDefinition> m_definitions;

		sc::ecs::Registry* m_registry = nullptr;
		sc::world::Level* m_currentLevel = nullptr;
	};
}
