#pragma once
#include <memory>

#include <engine/graphics/Texture2D.h>

#include "ZLayer.h"

namespace game::components {
	struct SpriteComponent {
		std::shared_ptr<sc::graphics::Texture2D> texture;
		ZLayer zlayer;
	};
}
