#pragma once
#include <memory>

#include <engine/graphics/Texture2D.h>

#include "ZLayer.h"

namespace game::components {
	struct SpriteComponent {
		std::shared_ptr<sc::graphics::Texture2D> texture;
		ZLayer zlayer;
		sf::IntRect rect;

		// specify rect
		SpriteComponent(const std::shared_ptr<sc::graphics::Texture2D>& texture, ZLayer zlayer, const sf::IntRect& rect)
			: texture(texture), zlayer(zlayer), rect(rect) {}
		// use full texture rect
		SpriteComponent(const std::shared_ptr<sc::graphics::Texture2D>& texture, ZLayer zlayer)
			: texture(texture), zlayer(zlayer), rect({ 0, 0 }, { (int32_t)texture->GetWidth(), (int32_t)texture->GetHeight() }) {}
	};
}
