#pragma once
#include <cstdint>
#include <filesystem>

namespace game {
	// window and display
	constexpr float NATIVE_WIDTH = 640.f;
    constexpr float NATIVE_HEIGHT = 360.f;
	constexpr float RENDER_SCALE = 2.f;
	constexpr uint32_t WINDOW_WIDTH = static_cast<uint32_t>(NATIVE_WIDTH * RENDER_SCALE);
	constexpr uint32_t WINDOW_HEIGHT = static_cast<uint32_t>(NATIVE_HEIGHT * RENDER_SCALE);

	// paths
	inline const std::filesystem::path ASSETS_DIR = std::filesystem::path("..") / ".." / "assets";
}
