#include <pch.h>
#include <engine/math/physics.h>

namespace sc::math {
	SweepResult SweptAABB(const sf::FloatRect& movingBox, const sf::Vector2f& velocity, const sf::FloatRect& staticBox) {
		SweepResult result;

		if (velocity.x == 0.0f && velocity.y == 0.0f) return result;

		// SFML 3 Compliant FloatRect Expansion
		sf::FloatRect expandedTarget(
			{ staticBox.position.x - movingBox.size.x, staticBox.position.y - movingBox.size.y },
			{ staticBox.size.x + movingBox.size.x, staticBox.size.y + movingBox.size.y }
		);

		// SFML 3 Ray Origin
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

		// Handle Floating Point overlap at start of frame
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
}