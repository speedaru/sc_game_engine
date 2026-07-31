#include <pch.h>
#include <blueprints/PlayerBlueprint.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void PlayerBlueprint::Build(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData) const {
		utils::assemblers::AttachTransform(entity, ldtkData);
		utils::assemblers::AttachSprite(entity, ldtkData, m_projectDir, m_textureCache);

		ecs::BoxColliderComponent collisions;
		collisions.hitboxes.push_back(ecs::Hitbox(-20.f, 0.f, 40.f, 25.f));

		// engine components
		entity.AddComponent<ecs::VelocityComponent>();
		entity.AddComponent<ecs::BoxColliderComponent>(collisions);
		
		// game componenets
		entity.AddComponent<components::PlayerTag>();
		entity.AddComponent<components::CharacterController>();
	}
}