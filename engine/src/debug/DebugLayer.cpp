#include <pch.h>
#include <engine/debug/DebugLayer.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <imgui.h>

#include <engine/debug/debug_system.h>
#include <engine/graphics/Camera2D.h>

namespace sc::debug {
	void DebugLayer::OnDetach() {
		Shutdown();
	}

	bool DebugLayer::OnEvent(const sf::Event& event) {
		if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
			if (key->code == TOGGLE_KEY) {
				ToggleOverlay();
				return true; // consume the toggle key
			}
		}

		// consume gui input
		const ImGuiIO& io = ImGui::GetIO();

		// consum key event
		if (io.WantCaptureKeyboard &&
			(event.is<sf::Event::KeyPressed>() ||
			 event.is<sf::Event::KeyReleased>() ||
			 event.is<sf::Event::TextEntered>()))
		{
			return true;
		}

		// consum mouse event
		if (io.WantCaptureMouse &&
			(event.is<sf::Event::MouseButtonPressed>() ||
			 event.is<sf::Event::MouseButtonReleased>() ||
			 event.is<sf::Event::MouseWheelScrolled>() ||
			 event.is<sf::Event::MouseMoved>()))
		{
			return true;
		}

		return false;
	}

	void DebugLayer::OnUpdate(float deltaTime) {
		// imgui widgets are built here
		DrawUI();
	}

	void DebugLayer::OnRender(sf::RenderWindow& window) {
		const DebugContext& ctx = GetContext();
		if (!ctx.IsValid()) {
			return;
		}

		m_draw.Begin(window, *ctx.camera);
		DrawOverlay(m_draw);
		m_draw.End();
	}
}

#endif
