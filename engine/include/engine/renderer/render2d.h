#pragma once
#include <vector>
#include <memory>

#include <SFML/Graphics/RenderTarget.hpp>

#include <engine/graphics/Texture2D.h>
#include <engine/graphics/Camera2D.h>
#include <engine/world/TileLayer.h>
#include <engine/world/YSortedTileLayer.h>

// render subsytem
namespace sc::renderer {
	// represents an individual quad (such as a sprite)
	struct QuadProps {
		sf::Vector2f position{};
		sf::Vector2f pivot{}; // 0 - 1 range
		std::shared_ptr<graphics::Texture2D> texture{};
		sf::IntRect textureRect{};
	};

	class TileMap;

	namespace render2d {
		void Initialize();
		void Shutdown();

		// begins the render pass and applies the camera view
		void BeginBatch(sf::RenderTarget& target, const graphics::Camera2D& camera);

		// ends the render pass and cleans up
		void EndBatch();

		// directly draws a tile layer to the bound target
		void DrawTileLayer(const sc::world::TileLayer& layer);

		// queues tiles in layer to be Y-Sorted with other quads (like entities)
		void SubmitYSortedTileLayer(const sc::world::YSortedTileLayer& layer);

		// queues a quad to be sorted and drawn
		void SubmitQuad(const QuadProps& quad);

		// sorts the current quad queue by y-axis and draws them, then clears the queue
		void FlushQuads();
	}
}
