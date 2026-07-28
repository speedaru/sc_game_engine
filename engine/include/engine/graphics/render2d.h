#pragma once
#include <memory>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>

#include <engine/graphics/Texture2D.h>
#include <engine/graphics/Camera2D.h>
#include <engine/graphics/Level.h>

// render subsytem
namespace sc::graphics {
	struct QuadProps {
		sf::Vector2f position{};
		sf::Vector2f pivot{}; // 0 - 1 range
		std::shared_ptr<Texture2D> texture{};
		sf::IntRect textureRect{};
		int16_t zIndex{};
	};

	class TileMap;

	namespace render2d {
		// lifecycle (called by Application)
		void Initialize();
		void Shutdown();

		// scene lifecycle (called by game)
		void BeginScene();
		void EndScene(sf::RenderWindow& window, const Camera2D& camera);

		// submission api
		void SubmitQuad(const QuadProps& quad);
		void SubmitMap(const TileMap& map);
		void SubmitLevel(const Level& level);
	}
}
