#include <pch.h>

#include <engine/ecs/ComponentAssemblers.h>

#include <components/ZLayer.h>
#include <blueprints/DragonBlueprint.h>
#include <blueprints/BlueprintRegistry.h>

namespace ecs = sc::ecs;

namespace game::blueprints {
	void DragonBlueprint::Build(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData, const fs::path& projectDir) const {
		ecs::assemblers::AttachTransform(entity, ldtkData);
		ecs::assemblers::AttachSprite(entity, ldtkData, projectDir, static_cast<int16_t>(ZLayer::Gameplay));
	}
}
