#pragma once

namespace sc::ecs {
	class Scene;
}

namespace game::systems {
	void UpdateCharacterMovement(sc::ecs::Scene& scene, float timeStep);
}
