#include <pch.h>

#include <LDtkLoader/Entity.hpp>

#include <engine/ecs/EntityFactory.h>
#include <engine/ecs/Entity.h>

namespace sc::ecs {
	void EntityFactory::Register(const std::string& identifier, std::unique_ptr<IBlueprint> blueprint) {
		m_blueprints[identifier] = std::move(blueprint);
	}

	Entity EntityFactory::Spawn(Scene& scene, const ldtk::Entity& ldtkData, const std::filesystem::path& projectDir) {
		Entity entity = scene.CreateEntity(ldtkData.getName());

		auto it = m_blueprints.find(ldtkData.getName());
		if (it != m_blueprints.end()) {
			it->second->Build(entity, ldtkData, projectDir);
		}
		else {
			LOG_W("EntityFactory: No blueprint registered for '%s'", ldtkData.getName().c_str());
		}

		return entity;
	}
}