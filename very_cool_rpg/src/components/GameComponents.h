#pragma once
#include <SFML/System/Vector2.hpp>

namespace game::components {
	// empty, used to identify the player
	struct PlayerTag {};

	struct CharacterController {
		float maxSpeed = 300.f;
		float acceleration = 2500.f;
		float friction = 2000.f;
		sf::Vector2f direction{}; // -1 to 1 on X and Y
	};
}
