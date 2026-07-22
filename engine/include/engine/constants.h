#pragma once
#include <cstdint>

#define CREATE_CONST(type, name, val) constexpr const type name = val;

namespace sc {
	// strings
	CREATE_CONST(char*, LOG_FILE_NAME, "log.txt");

	// floats
	CREATE_CONST(float, PHYSICS_HZ, 60.f);

	// ints
	CREATE_CONST(uint32_t, MAX_FPS, 165);
	CREATE_CONST(size_t, PREALLOCATED_RENDER_QUEUE, 1'000); // number of preallocated space for render queue in render2d
}
