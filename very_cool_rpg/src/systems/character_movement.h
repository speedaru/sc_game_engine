#pragma once

namespace sc::ecs {
	class Registry;
}

namespace game::systems {
	void UpdateCharacterMovement(sc::ecs::Registry& registry, float timeStep);
}
