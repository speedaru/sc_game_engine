#pragma once
#include <filesystem>
namespace fs = std::filesystem;

#include <engine/graphics/Texture2D.h>
#include <engine/utils/logging.h>

#include <SFML/Graphics.hpp>

namespace sc::graphics {
	class SFMLTexture2D : public Texture2D {
	public:
		SFMLTexture2D(const std::vector<uint8_t> data) {
			try {
				m_texture = sf::Texture(data.data(), data.size());
			}
			catch (const std::exception& e) {
				LOG_E("failed to load 2d texture from memory (%zu bytes): %s", data.size(), e.what());
			}
		}

		SFMLTexture2D(const fs::path& file) {
			if (!fs::exists(file)) {
				LOG_E("2d texture file doesn't exist: %s", file.string().c_str());
				return;
			}

			try {
				m_texture = sf::Texture(file);
			}
			catch (const std::exception& e) {
				LOG_E("failed to load 2d texture from path '%s': %s", file.string().c_str(), e.what());
			}
		}

		uint32_t GetWidth() const override { return m_texture.getSize().x; }
		uint32_t GetHeight() const override { return m_texture.getSize().y; }

		const sf::Texture& GetNativeTexture() const { return m_texture; }

	private:
		sf::Texture m_texture;
	};
}
