#pragma once

namespace sc::ecs {
	class Registry;
}

namespace game::systems {
	void UpdatePlayerInput(sc::ecs::Registry& registry);
}
