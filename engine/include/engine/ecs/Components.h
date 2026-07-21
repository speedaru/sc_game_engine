#pragma once
#include <string>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>

namespace sc::ecs {
	struct TagComponent {
		std::string tag;

		TagComponent() = default;
		TagComponent(const std::string& tag) : tag(tag) {}
	};

	struct PositionComponent {
		sf::Vector2f pos;
	};

	struct SpriteComponent {
		sf::Sprite sprite;
	};
}
