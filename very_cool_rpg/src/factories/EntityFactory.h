#pragma once
#include <memory>
#include <string>

#include <LDtkLoader/Entity.hpp>

#include <engine/world/Level.h>

#include <blueprints/IBlueprint.h>
#include <utils/assemblers/LdtkAssemblers.h>

namespace sc::ecs {
	class Entity;
	class Registry;
}

namespace game::factories {
	class EntityFactory {
	public:
		using TextureCache = utils::assemblers::TextureCache;

		void Register(const std::string& identifier, std::unique_ptr<blueprints::IBlueprint> blueprint);

		sc::ecs::Entity Spawn(sc::world::Level& level, sc::ecs::Registry& registry, const ldtk::Entity& ldtkData);
		
		TextureCache& GetTextureCache() { return m_textureCache; }

	private:
		TextureCache m_textureCache;
		std::unordered_map<std::string, std::unique_ptr<blueprints::IBlueprint>> m_blueprints;
	};
}
