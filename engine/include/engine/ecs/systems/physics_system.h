#pragma once
#include <engine/ecs/Registry.h>

namespace sc::ecs::physics_system  {
	void UpdateKinematics(Registry& scene, float timeStep);
}
