#pragma once
#include <unordered_map>
#include <functional>
#include <string>

#include <engine/ecs/Scene.h>
#include <engine/assets/LDtkLoader.h>

namespace sc::ecs {
	class EntityFactory {
	public:
		using BlueprintFn = std::function<Entity(Scene&, const sc::assets::EntitySpawnData&)>;
		
		void Register(const std::string& identifier, BlueprintFn blueprint);

		void Spawn(Scene& scene, const sc::assets::EntitySpawnData& spawnData);

	private:
		std::unordered_map<std::string, BlueprintFn> m_blueprints;
	};
}
