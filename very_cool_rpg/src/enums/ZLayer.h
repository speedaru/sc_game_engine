#pragma once
#include <cstdint>

namespace game {
	enum class ZLayer : int16_t {
		Background	= -100,	// background map, water
		Decals		= -50,	// blood stains, shadow		
		Gameplay	= 0,	// entities and other main things
		Foreground	= 100,	// clouds
		UI			= 1000,	// ui and hud elements
	};
}
