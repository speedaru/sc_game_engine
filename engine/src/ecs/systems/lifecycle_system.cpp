#include <pch.h>
#include <engine/ecs/systems/lifecycle_system.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/Components.h>
#include <engine/world/World.h>

namespace sc::ecs::lifecycle_system {
	void FlushDestroyed(Registry& registry, world::World& world) {
		for (auto ent : registry.TakePendingDestroy()) {
			// skip entities that are already destroyed
			if (!registry.IsEntityValid(ent)) continue;

			// unregister from the grid of the level the entity lives in, not the current one
			if (const auto* lvl = registry.TryGetComponent<LevelComponent>(ent)) {
				if (auto level = world.GetLevel(lvl->uid)) {
					physics_system::EntityUnregisterCollisions(level->GetSpatialGrid(), ent);
				}
				else {
					LOG_W("entity %u belongs to unknown level %d, so its grid entry can't be removed", static_cast<uint32_t>(ent), lvl->uid);
				}
			}

			registry.DestroyNow(ent);
		}
	}
}
