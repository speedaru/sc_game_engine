#pragma once

namespace sc::ecs {
	class Scene;

	namespace systems {
		void UpdateKinematics(Scene& scene, float timeStep);
	}
}
