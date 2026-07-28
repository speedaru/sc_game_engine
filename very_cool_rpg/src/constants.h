#pragma once
#include <filesystem>

#include <SFML/System/Vector2.hpp>

namespace fs = std::filesystem;


namespace game {
	// window and display
	constexpr float NATIVE_WIDTH = 640.f;
    constexpr float NATIVE_HEIGHT = 360.f;
	constexpr float RENDER_SCALE = 2.f;
	constexpr uint32_t WINDOW_WIDTH = static_cast<uint32_t>(NATIVE_WIDTH * RENDER_SCALE);
	constexpr uint32_t WINDOW_HEIGHT = static_cast<uint32_t>(NATIVE_HEIGHT * RENDER_SCALE);

	// paths
	const fs::path ASSETS_DIR = fs::path("..") / ".." / "assets";
}
