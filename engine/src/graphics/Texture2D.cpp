#include <pch.h>
#include <engine/graphics/Texture2D.h>
#include <engine/graphics/SFMLTexture2D.h>

namespace sc::graphics {
	std::shared_ptr<Texture2D> CreateTexture2D(const std::vector<uint8_t> data) {
		return std::make_shared<SFMLTexture2D>(data);
	}

	std::shared_ptr<Texture2D> CreateTexture2D(const fs::path& file) {
		return std::make_shared<SFMLTexture2D>(file);
	}
}
