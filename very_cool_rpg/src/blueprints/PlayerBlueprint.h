#pragma once
#include "includes.h"

namespace game::blueprints {
	class PlayerBlueprint : public IEntityBlueprint {
	public:
		void Build(sc::ecs::Entity& entity, const entities::SpawnParams& params) const override;
	};
}
