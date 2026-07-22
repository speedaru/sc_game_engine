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
	};

	struct SpriteComponent {
		std::shared_ptr<graphics::Texture2D> texture;
	};
}
