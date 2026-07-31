#include <pch.h>
#include <blueprints/DragonBlueprint.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void DragonBlueprint::Build(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData) const {
		utils::assemblers::AttachTransform(entity, ldtkData);
		utils::assemblers::AttachSprite(entity, ldtkData, m_projectDir, m_textureCache);

		// engine components
		entity.AddComponent<ecs::VelocityComponent>();

		// game componenets
		entity.AddComponent<components::CharacterController>();
	}
}
