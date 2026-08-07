#pragma once
#include <cstdint>

namespace game {
	enum class InputAction : int32_t {
		MoveUp,
		MoveDown,
		MoveLeft,
		MoveRight,
		SpawnEnt,
	};
}
