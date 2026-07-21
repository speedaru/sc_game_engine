#pragma once
#include <filesystem>
namespace fs = std::filesystem;

#define CREATE_CONST(type, name, val) constexpr const type name = val;

namespace game {
	// paths
	const fs::path ASSETS_DIR = fs::path("..") / ".." / "assets";
}
