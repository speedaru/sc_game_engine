#pragma once
#include <SFML/System/Vector2.hpp>

namespace sc::math {
	struct Hitbox {
		sf::Vector2f offset;
		sf::Vector2f size;

		Hitbox(float offsetX, float offsetY, float w, float h)
			: offset(offsetX, offsetY), size(w, h) {}

		// exact equality, on purpose: the collision shape palette dedups by comparing
		// whole box lists, and two shapes are only interchangeable if they are identical
		bool operator==(const Hitbox& other) const = default;
	};
}
