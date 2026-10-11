#pragma once
#include <cstdint>
#include <string>

#include <SFML/System/Vector2.hpp>

#include <types/LevelLocation.h>

namespace game::components {
	// tags are empty and just used to identify entities

	struct PlayerTag {};
	struct PickupTag {};

	struct TeleportComponent {
		LevelLocation target;
	};

	// identity of a placed entity that is the same on every run, unlike its handle.
	// only entities placed in the editor have one
	struct PersistentIdComponent {
		std::string id;
	};

	struct CharacterController {
		float maxSpeed = 200.f;
		float acceleration = 2000.f;
		float friction = 6000.f;
		sf::Vector2f direction{}; // -1 to 1 on X and Y
	};
}
