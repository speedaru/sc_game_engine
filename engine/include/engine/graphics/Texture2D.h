#pragma once
#include <string>
#include <memory>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

namespace sc::graphics {
	class Texture2D {
	public:
		virtual ~Texture2D() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
	};

	std::shared_ptr<Texture2D> CreateTexture2D(const std::vector<uint8_t> data);
	std::shared_ptr<Texture2D> CreateTexture2D(const fs::path& file);
}
