#pragma once
#include <cstdint>

namespace sc::ecs {
	class Registry;
}

namespace game::player_input {
	void UpdatePlayerInput(sc::ecs::Registry& registry, int32_t levelUid);
}
