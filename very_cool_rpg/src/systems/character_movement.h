#pragma once
#include <cstdint>

namespace sc::ecs {
	class Registry;
}

namespace game::character_movement {
	void UpdateCharacterMovement(sc::ecs::Registry& registry, int32_t levelUid, float timeStep);
}
