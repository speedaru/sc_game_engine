#pragma once
#include <LDtkLoader/Entity.hpp>

#include <engine/ecs/Scene.h>

namespace sc::ecs {
	class IBlueprint {
	public:
		virtual ~IBlueprint() = default;

		// factory calls this
		virtual void Build(Entity& entity, const ldtk::Entity& ldtkData, const fs::path& projectDir) const = 0;
	};
}
