#pragma once

namespace sc::math {
	// linear interpolation
	template <typename T>
	T Lerp(T start, T end, float alpha) {
		return start + (end - start) * alpha;
	}
}
