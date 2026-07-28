#pragma once
#include <filesystem>

#include <engine/ecs/Entity.h>
#include <engine/ecs/IBlueprint.h>

namespace fs = std::filesystem;

namespace game::blueprints {
	class DragonBlueprint : public sc::ecs::IBlueprint {
	public:
		void Build(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData, const fs::path& projectDir) const override;
	};
}
