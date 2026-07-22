#pragma once
#include <memory>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>

#include <engine/graphics/Texture2D.h>

// render subsytem
namespace sc::graphics {
	struct QuadProps {
		sf::Vector2f position{ 0.f, 0.f };
		std::shared_ptr<Texture2D> texture{ nullptr };
	};

	namespace render2d {
		// lifecycle (called by Application)
		void Initialize();
		void Shutdown();

		// scene lifecycle (called by game)
		void BeginScene();
		void EndScene(sf::RenderWindow& window);

		// submission api
		void Submit(const QuadProps& quad);
	}
}
