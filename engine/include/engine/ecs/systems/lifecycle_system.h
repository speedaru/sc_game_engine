#pragma once
#include <engine/ecs/Registry.h>

namespace sc::world {
	class World;
}

namespace sc::ecs::lifecycle_system {
	void FlushDestroyed(Registry& registry, world::World& world);
}
