#include <pch.h>
#include <engine/graphics/Texture2D.h>
#include <engine/graphics/SFMLTexture2D.h>

namespace sc::graphics {
	namespace {
		// a failed load is logged by the texture and leaves it empty. hand back null so the
		// caller can tell, instead of a valid texture that draws nothing
		std::shared_ptr<Texture2D> NullIfEmpty(std::shared_ptr<Texture2D> texture) {
			if (texture->GetWidth() == 0 || texture->GetHeight() == 0) return nullptr;
			return texture;
		}
	}

	std::shared_ptr<Texture2D> CreateTexture2D(const std::vector<uint8_t> data) {
		return NullIfEmpty(std::make_shared<SFMLTexture2D>(data));
	}

	std::shared_ptr<Texture2D> CreateTexture2D(const std::filesystem::path& file) {
		return NullIfEmpty(std::make_shared<SFMLTexture2D>(file));
	}
}
