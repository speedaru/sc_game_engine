#pragma once
#include <vector>

#include <engine/physics/Collisions.h>
#include <engine/ecs/Registry.h>

#include <layers/LayerRequest.h>

namespace game::collision_system {
	struct PairCooldown {
		sc::ecs::EntityHandle self;
		sc::ecs::EntityHandle other;
		float timeLeft;
	};

	// everything the contact handlers keep between steps or hand back to the caller
	struct ContactState {
		std::vector<PairCooldown> rateLimits;
		std::vector<LayerRequest> requests;
	};

	void DispatchContacts(
		sc::ecs::Registry& registry,
		const std::vector<sc::physics::Contact>& contacts,
		ContactState& state,
		float timeStep
	);
}
