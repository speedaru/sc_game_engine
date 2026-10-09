#pragma once
#include <SFML/System/Vector2.hpp>

namespace sc::math {
	struct Hitbox {
		sf::Vector2f offset;
		sf::Vector2f size;
		bool trigger;

		Hitbox(float offX, float offY, float w, float h, bool trigger = false)
			: offset(offX, offY), size(w, h), trigger(trigger) {}

		// exact equality so collision palette can dedup by comparing box lists
		bool operator==(const Hitbox& other) const = default;
	};
}
