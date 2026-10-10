#pragma once
#include <engine/physics/Collisions.h>
#include <engine/ecs/Registry.h>

namespace game::collision_system {
	struct PairCooldown {
		sc::ecs::EntityHandle self;
		sc::ecs::EntityHandle other;
		float timeLeft;
	};

	void DispatchContacts(
		sc::ecs::Registry& registry,
		const std::vector<sc::physics::Contact>& contacts,
		std::vector<PairCooldown>& rateLimits,
		float timeStep
	);
}
