#include <pch.h>
#include <engine/ecs/systems/lifecycle_system.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/Entity.h>
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

	void MoveToLevel(Registry& registry, world::World& world, EntityHandle ent, int32_t newLevelUid, sf::Vector2f pos) {
		auto& levelComponent = registry.GetComponent<LevelComponent>(ent);
		auto& trans = registry.GetComponent<TransformComponent>(ent);
		
		sc::world::Level* oldLevel = world.GetLevel(levelComponent.uid).get();
		sc::world::Level* newLevel = world.GetLevel(newLevelUid).get();

		if (!oldLevel || !newLevel) {
			if (!oldLevel) LOG_W("entity's %u current level component uid is invalid: %d", ent, levelComponent.uid);
			else if (!newLevel) LOG_W("new level uid doesn't exist: %d", newLevelUid);
			return;
		}

		// remove entity from old level grid
		physics_system::EntityUnregisterCollisions(oldLevel->GetSpatialGrid(), ent);

		// update entity to new level uid
		levelComponent.uid = newLevelUid;

		// update pos, prevpos and insert entity into new level
		trans.prevPos = trans.pos = pos;
		physics_system::EntityRegisterCollisions(newLevel->GetSpatialGrid(), ent, registry);
	}
}
