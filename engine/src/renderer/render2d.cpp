#include <pch.h>
#include <engine/renderer/render2d.h>
#include <engine/graphics/SFMLTexture2D.h>
#include <engine/utils/logging.h>
#include <engine/constants.h>

using namespace sc::graphics;

namespace sc::renderer::render2d {
	static sf::Texture s_emptyTexture;
    static bool s_initialized = false;

    static sf::RenderTarget* s_renderTarget = nullptr;
    static std::vector<QuadProps> s_quadQueue;
    static sf::Sprite s_sceneSprite(s_emptyTexture);

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
		s_initialized = false;
	}

	void BeginBatch(sf::RenderTarget& target, const Camera2D& camera) {
        assert(s_initialized);
        s_renderTarget = &target;
        s_renderTarget->setView(camera.GetView());
        s_quadQueue.clear();
    }

	void EndBatch() {
		assert(s_initialized);
		assert(s_renderTarget);

		// reset to default view to not mess up ui rendering
		s_renderTarget->setView(s_renderTarget->getDefaultView());
		s_renderTarget = nullptr;
	}

	void DrawTileLayer(const world::TileLayer& layer) {
        assert(s_initialized);
        assert(s_renderTarget);

        auto tileset = layer.GetTileSet();
        if (!tileset) return;

        auto texture = tileset->GetTexture();
        if (!texture) return;

		auto sfmlTexture = std::static_pointer_cast<SFMLTexture2D>(texture);

		sf::RenderStates states;
		states.texture = &sfmlTexture->GetNativeTexture(); 
		s_renderTarget->draw(layer.GetVertices(), states);
    }

    void SubmitYSortedTileLayer(const sc::world::YSortedTileLayer& layer) {
        assert(s_initialized);
        assert(s_renderTarget);

        const auto& tileSetTexture = layer.GetTileSet()->GetTexture();
        for (const auto& tile : layer.GetTiles()) {

            // calculate position of bottom left corner of the tile in level space
            float tileHeight = static_cast<float>(tile.textureRect.size.y);
            sf::Vector2f feetPosition = { tile.pixelPos.x, tile.pixelPos.y + tileHeight };

            SubmitQuad(QuadProps{
                .position = tile.pixelPos,
                .pivot = { 0.f, 1.f }, // pivot is at the feet
                .texture = tileSetTexture,
                .textureRect = tile.textureRect,
            });
        }
    }

    void SubmitQuad(const QuadProps& quad) {
        assert(s_initialized);
        assert(s_renderTarget);
        s_quadQueue.push_back(quad);
    }

    void FlushQuads() {
        assert(s_initialized);
        assert(s_renderTarget);

        if (s_quadQueue.empty()) return;

        // sort by y position (lower on screen = higher y value = drawn last)
        std::sort(s_quadQueue.begin(), s_quadQueue.end(), [](const QuadProps& a, const QuadProps& b) {
			float distanceToBottomA = (1.f - a.pivot.y) * static_cast<float>(a.textureRect.size.y);
			float distanceToBottomB = (1.f - b.pivot.y) * static_cast<float>(b.textureRect.size.y);

			float worldBottomA = a.position.y + distanceToBottomA;
			float worldBottomB = b.position.y + distanceToBottomB;

			return worldBottomA < worldBottomB;
        });

        for (const auto& quad : s_quadQueue) {
            if (!quad.texture) continue;

            auto sfmlTexture = std::static_pointer_cast<SFMLTexture2D>(quad.texture);

            s_sceneSprite.setTexture(sfmlTexture->GetNativeTexture(), true);
            s_sceneSprite.setTextureRect(quad.textureRect);

            sf::Vector2f spriteSize = s_sceneSprite.getLocalBounds().size;
            s_sceneSprite.setPosition(quad.position);
            s_sceneSprite.setOrigin({ quad.pivot.x * spriteSize.x, quad.pivot.y * spriteSize.y });

            s_renderTarget->draw(s_sceneSprite);
        }

        // clear queue for next EntityLayer
        s_quadQueue.clear();
    }
}