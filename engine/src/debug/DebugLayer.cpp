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
				return true; // consume: the game should never see the toggle key
			}
		}

		// swallow input the gui is currently using, so clicking a checkbox or typing in
		// a debug field doesn't also reach the game.
		//
		// CAVEAT: this only bites once the game reads input from events. sc::input is
		// still polling based (InputContext::IsActionPressed calls
		// sf::Keyboard::isKeyPressed directly), so consuming a key event here does not
		// currently stop the player walking while you type
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
		// imgui widgets are built here rather than in OnRender because they have to sit
		// between Application's ImGui::SFML::Update and ImGui::SFML::Render
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
