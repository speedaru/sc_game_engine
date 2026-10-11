#pragma once
#include <vector>

#include <engine/physics/Collisions.h>
#include <engine/ecs/EntityHandle.h>

#include <layers/LayerRequest.h>

namespace sc::ecs {
	class Registry;
}

namespace game::collision_system {
	struct PairCooldown {
		sc::ecs::EntityHandle self;
		sc::ecs::EntityHandle other;
		float timeLeft;
	};

	// everything the contact handlers keep between steps
	struct ContactState {
		std::vector<PairCooldown> rateLimits;
	};

	// handlers append to requests, the caller owns the queue and applies it afterwards
	void DispatchContacts(
		sc::ecs::Registry& registry,
		const std::vector<sc::physics::Contact>& contacts,
		ContactState& state,
		std::vector<LayerRequest>& requests,
		float timeStep
	);
}
