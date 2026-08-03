#include <pch.h>
#include <blueprints/PlayerBlueprint.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void PlayerBlueprint::Build(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData) const {
		utils::assemblers::AttachTransform(entity, ldtkData);
		utils::assemblers::AttachSprite(entity, ldtkData, m_projectDir, m_textureCache);

		// engine components
		entity.AddComponent<ecs::VelocityComponent>();
		
		// game componenets
		entity.AddComponent<components::PlayerTag>();
		entity.AddComponent<components::CharacterController>();
	}
}