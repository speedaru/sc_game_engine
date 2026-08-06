#pragma once
#include <engine/debug/debug_config.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf { class RenderTarget; }
namespace sc::graphics { class Camera2D; }

namespace sc::debug {
	// batched world space primitives for overlay drawing. every coordinate is in level
	// space - Begin applies the camera view, so modules never touch sf::View themselves.
	//
	// batched rather than one RectangleShape per call (which is what GameplayLayer's
	// hand rolled collision debug does today): a spatial grid overlay draws hundreds of
	// cells plus a line per entity, and that's a draw call each.
	//
	// this lives in debug/ instead of render2d because render2d is built around
	// textured, y sorted quads and hosting this would mean carving SC_ENABLE_DEBUG_TOOLS
	// holes through it. keeping it here means the whole thing vanishes in release
	class DebugDraw {
	public:
		DebugDraw();

		// binds the target and applies the camera view. all draw calls must sit
		// between Begin and End
		void Begin(sf::RenderTarget& target, const graphics::Camera2D& camera);

		// flushes the batches and restores the target's default view
		void End();

		void Line(sf::Vector2f a, sf::Vector2f b, sf::Color color);
		void Rect(const sf::FloatRect& rect, sf::Color color); // outline only
		void FilledRect(const sf::FloatRect& rect, sf::Color color);
		void Cross(sf::Vector2f center, float halfSize, sf::Color color);

	private:
		// fills go down before outlines, so a rect's border lands on top of its own fill
		void Flush();

	private:
		sf::RenderTarget* m_target = nullptr;
		sf::VertexArray m_lines;
		sf::VertexArray m_triangles;
	};
}

#endif
