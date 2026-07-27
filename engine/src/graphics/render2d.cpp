#include <pch.h>
#include <engine/graphics/render2d.h>
#include <engine/graphics/SFMLTexture2D.h>
#include <engine/graphics/TileMap.h>
#include <engine/utils/logging.h>
#include <engine/constants.h>

namespace sc::graphics::render2d {
	static sf::Texture s_emptyTexture;
	static bool s_initialized = false;

	static std::vector<QuadProps> s_quadQueue;
	static std::vector<const TileMap*> s_mapQueue;
	static sf::Sprite s_sceneSprite(s_emptyTexture); // reuse same sprite to render everything

	void Initialize() {
		assert(!s_initialized);
		LOG_I("initializing render2d subsystem");

		// preallocate a lot of memory to avoid reallocations
		s_quadQueue.reserve(PREALLOCATED_QUAD_QUEUE);
		s_initialized = true;
	}

	void Shutdown() {
		assert(s_initialized);
		LOG_I("shutting down render2d subsystem");

		s_quadQueue.clear();
		s_mapQueue.clear();
		s_initialized = false;
	}

	void BeginScene() {
		assert(s_initialized);
		s_quadQueue.clear();
		s_mapQueue.clear();
	}

	void EndScene(sf::RenderWindow& window, const Camera2D& camera) {
		assert(s_initialized);
		std::sort(s_quadQueue.begin(), s_quadQueue.end(), [](const QuadProps& a, const QuadProps& b) {
			if (a.zIndex == b.zIndex) {
				// if on same layer sort by Y
				return a.position.y < b.position.y;
			}
			// otherwise sort by z index
			return a.zIndex < b.zIndex;
		});

		window.setView(camera.GetView());

		// render maps
		for (const TileMap* map : s_mapQueue) {
			if (map->GetTileSet() && map->GetTileSet()->GetTexture()) {
				auto sfmlTexture = std::static_pointer_cast<SFMLTexture2D>(map->GetTileSet()->GetTexture());

				sf::RenderStates states;
				states.texture = &sfmlTexture->GetNativeTexture(); // pass texture to vertices
				window.draw(map->GetVertices(), states);
			}
		}

		// render quads
		for (const auto& quad : s_quadQueue) {
			if (!quad.texture) continue;

			auto sfmlTexture = std::static_pointer_cast<SFMLTexture2D>(quad.texture);
			s_sceneSprite.setTexture(sfmlTexture->GetNativeTexture(), true);
			s_sceneSprite.setTextureRect(quad.textureRect);
			s_sceneSprite.setPosition(quad.position);

			window.draw(s_sceneSprite);
		}

		window.setView(window.getDefaultView());
	}

	void SubmitQuad(const QuadProps& quad) {
		assert(s_initialized);
		s_quadQueue.push_back(quad);
	}

	void SubmitMap(const TileMap& map) {
		assert(s_initialized);
		s_mapQueue.push_back(&map);
	}

	void SubmitLevel(const LevelGraphics& level) {
		for (const auto layer : level.GetLayers()) {
			SubmitMap(*layer);
		}
	}
}