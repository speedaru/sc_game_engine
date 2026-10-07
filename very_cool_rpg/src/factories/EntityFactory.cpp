#include <pch.h>
#include <factories/EntityFactory.h>

#include <engine/ecs/Components.h>
#include <engine/ecs/Entity.h>
#include <engine/ecs/Registry.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/utils/logging.h>

namespace ecs = sc::ecs;

namespace game::factories {
	void EntityFactory::Register(EntityType type, std::unique_ptr<blueprints::IEntityBlueprint> blueprint) {
		if (!blueprint) {
			LOG_W("tried to register a null blueprint for '%llu'", static_cast<uint64_t>(type));
			return;
		}

		m_blueprints[type] = std::move(blueprint);
	}

	void EntityFactory::SetDefinition(EntityType type, entities::EntityDefinition definition) {
		m_definitions[type] = std::move(definition);
	}

	ecs::Entity EntityFactory::Spawn(EntityType type, const entities::SpawnParams& params) {
		if (!m_registry || !m_currentLevel) {
			if (!m_registry) LOG_E("EntityFactory::Spawn called before SetRegistry");
			if (!m_currentLevel) LOG_E("can't spawn entity because current level is not set");

			return ecs::NULL_ENT;
		}

		// find blueprint
		auto blueprintIt = m_blueprints.find(type);
		if (blueprintIt == m_blueprints.end()) {
			LOG_W("no blueprint registered for entity type %llu", static_cast<uint64_t>(type));
			return ecs::NULL_ENT;
		}

		// find entity definition
		auto definitionIt = m_definitions.find(type);
		if (definitionIt == m_definitions.end()) {
			LOG_W("no definition loaded for entity type %llu", static_cast<uint64_t>(type));
			return ecs::NULL_ENT;
		}

		// create entity
		const entities::EntityDefinition& definition = definitionIt->second;
		ecs::Entity entity = m_registry->CreateEntity(definition.debugName);

		// add transform component
		entity.AddComponent<ecs::TransformComponent>(params.position, definition.pivot);

		// add level component
		entity.AddComponent<ecs::LevelComponent>(m_currentLevel->GetUid());

		// add texture component if present
		if (definition.texture) {
			entity.AddComponent<ecs::SpriteComponent>(definition.texture, params.layerUid, definition.textureRect);
		}

		// add collider component
		if (!definition.hitboxes.empty()) {
			auto& collider = entity.AddComponent<ecs::BoxColliderComponent>();
			collider.hitboxes = definition.hitboxes;
		}

		// type specific components
		blueprintIt->second->Build(entity, params);

		// register collisions with level
		ecs::physics_system::EntityRegisterCollisions(m_currentLevel->GetSpatialGrid(), entity);

		return entity;
	}
}
