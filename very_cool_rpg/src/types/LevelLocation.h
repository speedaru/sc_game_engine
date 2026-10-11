#pragma once
#include <cstdint>

#include <SFML/System/Vector2.hpp>

namespace game {
	// a position inside one level
	struct LevelLocation {
		int32_t levelUid;
		sf::Vector2f pos;
	};
}
