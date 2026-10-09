#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

namespace sc::math {
	struct SweepResult {
		float time = 1.f; // 1 means no collision during this frame
		sf::Vector2f normal{}; // which side did we hit ? { -1, 0 } = right wall

		bool IsHit() const { return time < 1.f; }
	};

	SweepResult SweptAABB(const sf::FloatRect& movingBox, const sf::Vector2f& velocity, const sf::FloatRect& staticBox);
}
