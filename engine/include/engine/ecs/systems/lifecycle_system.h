#pragma once
#include <engine/ecs/Registry.h>

namespace sc::world {
	class Level;
}

namespace sc::ecs::lifecycle_system {
	void FlushDestroyed(Registry& registry, world::Level& level);
}
