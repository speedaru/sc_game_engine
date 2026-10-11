#pragma once
#include <cstdint>

#include <SFML/System/Vector2.hpp>

#include <engine/ecs/EntityHandle.h>

namespace sc::world {
	class World;
}

namespace sc::ecs {
	class Registry;
}

namespace sc::ecs::lifecycle_system {
	void FlushDestroyed(Registry& registry, world::World& world);

	void MoveToLevel(Registry& registry, world::World& world, EntityHandle ent, int32_t newLevelUid, sf::Vector2f pos);
}
