#pragma once
#include <SFML/System/Vector2.hpp>

namespace game::components {
	// empty, used to identify the player
	struct PlayerTag {};

	struct CharacterController {
		float maxSpeed = 250.f;
		float acceleration = 2000.f;
		float friction = 5000.f;
		sf::Vector2f direction{}; // -1 to 1 on X and Y
	};
}
