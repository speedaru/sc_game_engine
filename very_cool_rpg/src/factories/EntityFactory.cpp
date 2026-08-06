#include <pch.h>

#include <engine/ecs/Entity.h>
#include <engine/ecs/Registry.h>
#include <engine/ecs/systems/physics_system.h>

#include <factories/EntityFactory.h>

namespace ecs = sc::ecs;

namespace game::factories {
	void EntityFactory::Register(const std::string& identifier, std::unique_ptr<blueprints::IBlueprint> blueprint) {
		m_blueprints[identifier] = std::move(blueprint);
	}

	ecs::Entity EntityFactory::Spawn(sc::world::Level& level, ecs::Registry& registry, const ldtk::Entity& ldtkData) {
		ecs::Entity entity = registry.CreateEntity(ldtkData.getName());

		auto it = m_blueprints.find(ldtkData.getName());
		if (it != m_blueprints.end()) {
			// build the entity components
			it->second->Build(entity, ldtkData);

			// register entity collisions in the level
			ecs::physics_system::RegisterEntityCollisions(level.GetSpatialGrid(), entity);
		}
		else {
			LOG_W("GameEntityFactory: No blueprint registered for '%s'", ldtkData.getName().c_str());
		}

		return entity;
	}
}