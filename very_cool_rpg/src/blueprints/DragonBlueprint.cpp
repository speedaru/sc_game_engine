#include <pch.h>
#include <blueprints/DragonBlueprint.h>
#include <enums/ZLayer.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void DragonBlueprint::Build(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData) const {
		utils::assemblers::AttachTransform(entity, ldtkData);
		utils::assemblers::AttachSprite(entity, ldtkData, m_projectDir, m_textureCache, static_cast<int16_t>(ZLayer::Gameplay));

		// engine components
		entity.AddComponent<ecs::VelocityComponent>();

		// game componenets
		entity.AddComponent<components::CharacterController>();
	}
}
