#include <pch.h>
#include <engine/debug/DebugDraw.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <SFML/Graphics/RenderTarget.hpp>

#include <engine/graphics/Camera2D.h>

namespace sc::debug {
	DebugDraw::DebugDraw()
		: m_lines(sf::PrimitiveType::Lines),
		m_triangles(sf::PrimitiveType::Triangles)
	{
	}

	void DebugDraw::Begin(sf::RenderTarget& target, const graphics::Camera2D& camera) {
		m_target = &target;
		m_target->setView(camera.GetView());
	}

	void DebugDraw::End() {
		if (!m_target) return;

		Flush();

		// leave the target as we found it so nothing downstream inherits the camera view
		m_target->setView(m_target->getDefaultView());
		m_target = nullptr;
	}

	void DebugDraw::Line(sf::Vector2f a, sf::Vector2f b, sf::Color color) {
		m_lines.append(sf::Vertex{ a, color });
		m_lines.append(sf::Vertex{ b, color });
	}

	void DebugDraw::Rect(const sf::FloatRect& rect, sf::Color color) {
		const sf::Vector2f tl = rect.position;
		const sf::Vector2f tr = { rect.position.x + rect.size.x, rect.position.y };
		const sf::Vector2f br = rect.position + rect.size;
		const sf::Vector2f bl = { rect.position.x, rect.position.y + rect.size.y };

		Line(tl, tr, color);
		Line(tr, br, color);
		Line(br, bl, color);
		Line(bl, tl, color);
	}

	void DebugDraw::FilledRect(const sf::FloatRect& rect, sf::Color color) {
		const sf::Vector2f tl = rect.position;
		const sf::Vector2f tr = { rect.position.x + rect.size.x, rect.position.y };
		const sf::Vector2f br = rect.position + rect.size;
		const sf::Vector2f bl = { rect.position.x, rect.position.y + rect.size.y };

		// two triangles per quad
		m_triangles.append(sf::Vertex{ tl, color });
		m_triangles.append(sf::Vertex{ tr, color });
		m_triangles.append(sf::Vertex{ br, color });

		m_triangles.append(sf::Vertex{ tl, color });
		m_triangles.append(sf::Vertex{ br, color });
		m_triangles.append(sf::Vertex{ bl, color });
	}

	void DebugDraw::Cross(sf::Vector2f center, float halfSize, sf::Color color) {
		Line({ center.x - halfSize, center.y }, { center.x + halfSize, center.y }, color);
		Line({ center.x, center.y - halfSize }, { center.x, center.y + halfSize }, color);
	}

	void DebugDraw::Flush() {
		if (m_triangles.getVertexCount() > 0) {
			m_target->draw(m_triangles);
			m_triangles.clear();
		}

		if (m_lines.getVertexCount() > 0) {
			m_target->draw(m_lines);
			m_lines.clear();
		}
	}
}

#endif
