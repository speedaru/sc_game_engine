#pragma once

namespace sc::ecs { class Entity; }
namespace game::entities { struct SpawnParams; }

namespace game::blueprints {
	// contains the type specific entity components
	// Build is called by the EntityFactory
	// EntityFactory already attaches transform, sprite and collider components if they are present
	class IEntityBlueprint {
	public:
		virtual ~IEntityBlueprint() = default;

		virtual void Build(sc::ecs::Entity& entity, const entities::SpawnParams& params) const = 0;
	};
}
