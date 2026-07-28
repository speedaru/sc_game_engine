#pragma once

namespace sc::ecs {
	class Scene;
}

namespace game::systems {
	void UpdatePlayerInput(sc::ecs::Scene& scene);
}
