#include <pch.h>
#include <engine/math/physics.h>

namespace sc::math {
	SweepResult SweptAABB(const sf::FloatRect& movingBox, const sf::Vector2f& velocity, const sf::FloatRect& staticBox) {
		SweepResult result;

		if (velocity.x == 0.0f && velocity.y == 0.0f) return result;

		sf::FloatRect expandedTarget(
			{ staticBox.position.x - movingBox.size.x, staticBox.position.y - movingBox.size.y },
			{ staticBox.size.x + movingBox.size.x, staticBox.size.y + movingBox.size.y }
		);

		sf::Vector2f rayOrigin = movingBox.position;

		float tNearX, tFarX, tNearY, tFarY;

		if (velocity.x == 0.0f) {
			if (rayOrigin.x >= expandedTarget.position.x && rayOrigin.x <= expandedTarget.position.x + expandedTarget.size.x) {
				tNearX = std::numeric_limits<float>::lowest();
				tFarX = std::numeric_limits<float>::max();
			}
			else {
				return result;
			}
		}
		else {
			tNearX = (expandedTarget.position.x - rayOrigin.x) / velocity.x;
			tFarX = (expandedTarget.position.x + expandedTarget.size.x - rayOrigin.x) / velocity.x;
			if (tNearX > tFarX) std::swap(tNearX, tFarX);
		}

		if (velocity.y == 0.0f) {
			if (rayOrigin.y >= expandedTarget.position.y && rayOrigin.y <= expandedTarget.position.y + expandedTarget.size.y) {
				tNearY = std::numeric_limits<float>::lowest();
				tFarY = std::numeric_limits<float>::max();
			}
			else {
				return result;
			}
		}
		else {
			tNearY = (expandedTarget.position.y - rayOrigin.y) / velocity.y;
			tFarY = (expandedTarget.position.y + expandedTarget.size.y - rayOrigin.y) / velocity.y;
			if (tNearY > tFarY) std::swap(tNearY, tFarY);
		}

		if (tNearX > tFarY || tNearY > tFarX) return result;

		float tHitNear = std::max(tNearX, tNearY);
		float tHitFar = std::min(tFarX, tFarY);

		if (tHitFar < 0.0f || tHitNear >= 1.0f) return result;

		// handle floating point overlap at start of frame
		if (tHitNear < 0.0f) {
			result.time = 0.0f;
			if (tNearX > tNearY) {
				result.normal.x = (velocity.x < 0.0f) ? 1.0f : -1.0f;
				result.normal.y = 0.0f;
			}
			else {
				result.normal.x = 0.0f;
				result.normal.y = (velocity.y < 0.0f) ? 1.0f : -1.0f;
			}
			return result;
		}

		result.time = tHitNear;

		if (tNearX > tNearY) {
			result.normal.x = (velocity.x < 0.0f) ? 1.0f : -1.0f;
			result.normal.y = 0.0f;
		}
		else {
			result.normal.x = 0.0f;
			result.normal.y = (velocity.y < 0.0f) ? 1.0f : -1.0f;
		}

		return result;
	}

	sf::Vector2f Penetration(const sf::FloatRect& moving, const sf::FloatRect& fixed) {
		const float left = (fixed.position.x + fixed.size.x) - moving.position.x; // push right by this
		const float right = (moving.position.x + moving.size.x) - fixed.position.x; // push left by this
		const float down = (fixed.position.y + fixed.size.y) - moving.position.y;
		const float up = (moving.position.y + moving.size.y) - fixed.position.y;

		if (left <= 0.f || right <= 0.f || down <= 0.f || up <= 0.f) return {};

		const float pushX = (left < right) ? left : -right;
		const float pushY = (down < up) ? down : -up;

		return (std::abs(pushX) < std::abs(pushY)) ? sf::Vector2f{ pushX, 0.f } : sf::Vector2f{ 0.f, pushY };
	}
}