#include <pch.h>
#include <engine/debug/DebugDraw.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <imgui.h>

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

	void DebugDraw::Arrow(sf::Vector2f from, sf::Vector2f to, sf::Color color, float headSize) {
		Line(from, to, color);

		const sf::Vector2f delta = to - from;
		const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);

		// degenerate: no direction to point a head in
		if (length < 0.0001f) return;

		const sf::Vector2f dir = delta / length;
		const sf::Vector2f normal = { -dir.y, dir.x };
		const sf::Vector2f base = to - dir * headSize;

		Line(to, base + normal * (headSize * 0.5f), color);
		Line(to, base - normal * (headSize * 0.5f), color);
	}

	void DebugDraw::Text(sf::Vector2f center, sf::Color color, const std::string& text) {
		if (!m_target || text.empty()) return;

		// routed through imgui's draw list rather than sf::Text for two reasons. there is
		// no font asset in this project, and sf::Text needs one. and imgui's glyphs are
		// screen space sized, so labels stay legible at any zoom, where world space text
		// turns to mush the moment you zoom out - which is exactly when you want to read
		// per cell counts.
		//
		// safe because Text is only reachable from DebugLayer::OnRender, which runs
		// between Application's ImGui::SFML::Update and ImGui::SFML::Render. that also
		// means text always lands on top of the vertex batches, since imgui renders after
		// every layer has drawn
		const sf::Vector2i pixel = m_target->mapCoordsToPixel(center);
		const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

		// mapCoordsToPixel gives the centre; AddText wants the top left
		const ImVec2 topLeft{
			static_cast<float>(pixel.x) - textSize.x * 0.5f,
			static_cast<float>(pixel.y) - textSize.y * 0.5f
		};

		ImGui::GetBackgroundDrawList()->AddText(
			topLeft,
			IM_COL32(color.r, color.g, color.b, color.a),
			text.c_str()
		);
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
