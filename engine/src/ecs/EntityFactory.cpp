#include <pch.h>
#include <engine/ecs/EntityFactory.h>
#include <engine/ecs/Entity.h>

namespace sc::ecs {
	void EntityFactory::Register(const std::string& identifier, BlueprintFn blueprint) {
		m_blueprints[identifier] = blueprint;
	}
	
	void EntityFactory::Spawn(Scene& scene, const sc::assets::EntitySpawnData& spawnData) {
		auto it = m_blueprints.find(spawnData.identifier);
		if (it != m_blueprints.end()) {
			it->second(scene, spawnData);
		}
		else {
			LOG_W("no blueprint registered for identifier %s", spawnData.identifier);
		}
	}
}