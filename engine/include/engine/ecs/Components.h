#pragma once
#include <string>
#include <vector>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>

#include <engine/graphics/Texture2D.h>

namespace sc::ecs {
	struct TagComponent {
		std::string tag;
	};

	struct TransformComponent {
		sf::Vector2f pos;
		sf::Vector2f pivot{}; // 0 - 1 range
	};

	struct VelocityComponent {
		sf::Vector2f velocity{}; // movement speed
	};

	struct SpriteComponent {
		std::shared_ptr<sc::graphics::Texture2D> texture;
		int32_t layerUid;
		sf::IntRect rect;

		// specify rect
		SpriteComponent(const std::shared_ptr<sc::graphics::Texture2D>& texture, int32_t layerUid, const sf::IntRect& rect)
			: texture(texture), layerUid(layerUid), rect(rect) {}

		// use full texture rect
		SpriteComponent(const std::shared_ptr<sc::graphics::Texture2D>& texture, int32_t layerUid)
			: SpriteComponent(texture, layerUid, sf::IntRect({ 0, 0 }, { (int32_t)texture->GetWidth(), (int32_t)texture->GetHeight() })) {}
	};

	struct Hitbox {
		sf::Vector2f offset;
		sf::Vector2f size;

		Hitbox(float offsetX, float offsetY, float w, float h)
			: offset(offsetX, offsetY), size(w, h) {}
	};

	struct BoxColliderComponent {
		std::vector<Hitbox> hitboxes;

		BoxColliderComponent() = default;
	};
}
