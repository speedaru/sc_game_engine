#include <pch.h>
#include <engine/graphics/render2d.h>
#include <engine/graphics/SFMLTexture2D.h>
#include <engine/utils/logging.h>
#include <engine/constants.h>

namespace sc::graphics::render2d {
	static std::vector<QuadProps> s_renderQueue;
	static sf::Texture s_emptyTexture;
	static sf::Sprite s_sprite(s_emptyTexture); // reuse same sprite to render everything
	static bool s_initialized = false;

	void Initialize() {
		assert(!s_initialized);
		LOG_I("initializing render2d subsystem");

		// preallocate a lot of memory to avoid reallocations
		s_renderQueue.reserve(PREALLOCATED_RENDER_QUEUE);
		s_initialized = true;
	}

	void Shutdown() {
		assert(s_initialized);
		LOG_I("shutting down render2d subsystem");

		s_renderQueue.clear();
		s_initialized = false;
	}

	void BeginScene() {
		assert(s_initialized);

		s_renderQueue.clear();
	}

	void EndScene(sf::RenderWindow& window) {
		assert(s_initialized);

		for (const auto& quad : s_renderQueue) {
			if (!quad.texture) continue;

			auto sfmlTexture = std::static_pointer_cast<SFMLTexture2D>(quad.texture);
			s_sprite.setTexture(sfmlTexture->GetNativeTexture(), true);
			s_sprite.setPosition(quad.position);

			window.draw(s_sprite);
		}
	}

	void Submit(const QuadProps& quad) {
		assert(s_initialized);

		s_renderQueue.push_back(quad);
	}
}