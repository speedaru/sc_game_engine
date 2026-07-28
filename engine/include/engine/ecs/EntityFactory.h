#pragma once
#include <unordered_map>
#include <functional>
#include <string>

#include <engine/ecs/IBlueprint.h>
#include <engine/ecs/ComponentAssemblers.h>
#include <engine/loader/types.h>

namespace ldtk {
	class Entity;
}

namespace sc::ecs {
	class EntityFactory {
	public:
		void Register(const std::string& identifier, std::unique_ptr<IBlueprint> blueprint);

		Entity Spawn(Scene& scene, const ldtk::Entity& ldtkData, const std::filesystem::path& projectDir);

	private:
		std::unordered_map<std::string, std::unique_ptr<IBlueprint>> m_blueprints;
	};
}
