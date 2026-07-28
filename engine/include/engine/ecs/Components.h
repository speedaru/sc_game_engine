#pragma once
#include <string>

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
		int16_t zlayer;
		sf::IntRect rect;

		// specify rect
		SpriteComponent(const std::shared_ptr<sc::graphics::Texture2D>& texture, int16_t zlayer, const sf::IntRect& rect)
			: texture(texture), zlayer(zlayer), rect(rect) {}

		// use full texture rect
		SpriteComponent(const std::shared_ptr<sc::graphics::Texture2D>& texture, int16_t zlayer)
			: texture(texture), zlayer(zlayer),
			rect({ 0, 0 }, { (int32_t)texture->GetWidth(), (int32_t)texture->GetHeight() }) {}
	};
}
