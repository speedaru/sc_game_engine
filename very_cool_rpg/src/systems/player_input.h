#pragma once

namespace sc::ecs {
	class Registry;
}

namespace game::player_input {
	void UpdatePlayerInput(sc::ecs::Registry& registry, int32_t levelUid);
}
