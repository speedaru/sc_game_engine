#pragma once
#include <optional>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include <engine/physics/Collisions.h>
#include <engine/ecs/Registry.h>

namespace game::collision_system {
	struct PairCooldown {
		sc::ecs::EntityHandle self;
		sc::ecs::EntityHandle other;
		float timeLeft;
	};

	// handlers can't switch level while the contacts are being iterated, so they record
	// the request and the owner of the state applies it after DispatchContacts returns
	struct TeleportRequest {
		int32_t levelUid;
		sf::Vector2f pos;
	};

	// everything the contact handlers keep between steps or hand back to the caller
	struct ContactState {
		std::vector<PairCooldown> rateLimits;
		std::optional<TeleportRequest> teleport;
	};

	void DispatchContacts(
		sc::ecs::Registry& registry,
		const std::vector<sc::physics::Contact>& contacts,
		ContactState& state,
		float timeStep
	);
}
