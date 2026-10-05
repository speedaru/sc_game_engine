#include <pch.h>
#include <engine/ecs/systems/lifecycle_system.h>
#include <engine/ecs/systems/physics_system.h>

namespace sc::ecs::lifecycle_system {
	void FlushDestroyed(Registry& registry, world::Level& level) {
		for (auto ent : registry.TakePendingDestroy()) {
			// skip entities that are already destroyed
			if (!registry.GetRegistry().valid(ent)) continue;

			physics_system::EntityUnregisterCollisions(level.GetSpatialGrid(), ent);

			// unregister from registry
			registry.GetRegistry().destroy(ent);
		}
	}
}
