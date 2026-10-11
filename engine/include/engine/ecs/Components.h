#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <engine/math/Hitbox.h>

namespace sc::graphics {
	class Texture2D;
}

namespace sc::ecs {
	struct TagComponent {
		std::string tag;
	};

	struct TransformComponent {
		sf::Vector2f pos; // current position
		sf::Vector2f prevPos; // previous position for rendering interpolation
		sf::Vector2f pivot{}; // 0 - 1 range

		TransformComponent(sf::Vector2f pos, sf::Vector2f pivot)
			: pos(pos), prevPos(pos), pivot(pivot) {}
	};

	struct VelocityComponent {
		sf::Vector2f velocity{}; // movement speed
	};

	struct SpriteComponent {
		std::shared_ptr<sc::graphics::Texture2D> texture;
		int32_t layerUid;
		sf::IntRect rect;

		SpriteComponent(const std::shared_ptr<sc::graphics::Texture2D>& texture, int32_t layerUid, const sf::IntRect& rect)
			: texture(texture), layerUid(layerUid), rect(rect) {}
	};

	struct BoxColliderComponent {
		std::vector<math::Hitbox> hitboxes;

		BoxColliderComponent() = default;
	};

	struct LevelComponent {
		int32_t uid;
	};
}
