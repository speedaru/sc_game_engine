#pragma once
#include <LDtkLoader/Entity.hpp>

#include <engine/ecs/Scene.h>

namespace game::blueprints {
	class IBlueprint {
    public:
        virtual ~IBlueprint() = default;

        virtual void Build(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData) const = 0;
    };
}
