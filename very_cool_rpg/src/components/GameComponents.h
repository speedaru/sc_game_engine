#pragma once
#include <cstdint>

#include <SFML/System/Vector2.hpp>

namespace game::components {
	// tags are empty and just used to identify entities

	struct PlayerTag {};
	struct PickupTag {};

	struct TeleportComponent {
		// target level and position
		int32_t levelUid;
		sf::Vector2f pos;
	};

	struct CharacterController {
		float maxSpeed = 200.f;
		float acceleration = 2000.f;
		float friction = 6000.f;
		sf::Vector2f direction{}; // -1 to 1 on X and Y
	};
}
