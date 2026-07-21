#pragma once
#include <cstdint>

#define CREATE_CONST(type, name, val) constexpr const type name = val;

namespace sc {
	CREATE_CONST(float, PHYSICS_HZ, 60.f);
	CREATE_CONST(uint32_t, MAX_FPS, 165);
}
