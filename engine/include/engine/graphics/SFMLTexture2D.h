#pragma once
#include <filesystem>
namespace fs = std::filesystem;

#include <engine/graphics/Texture2D.h>

#include <SFML/Graphics.hpp>

namespace sc::graphics {
	class SFMLTexture2D : public Texture2D {
	public:
		SFMLTexture2D(const std::vector<uint8_t> data)
			: m_texture(data.data(), data.size()) {}

		SFMLTexture2D(const fs::path& file)
			: m_texture(file) {}

		uint32_t GetWidth() const override { return m_texture.getSize().x; }
		uint32_t GetHeight() const override { return m_texture.getSize().y; }

		const sf::Texture& GetNativeTexture() const { return m_texture; }

	private:
		sf::Texture m_texture;
	};
}
