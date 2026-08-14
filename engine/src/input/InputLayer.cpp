#include <pch.h>
#include <engine/input/InputLayer.h>

#include <engine/input/InputState.h>

namespace sc::input {
	bool InputLayer::OnEvent(const sf::Event& event) {
		if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
			internal::OnKeyDown(key->code);
		}
		else if (const auto* key = event.getIf<sf::Event::KeyReleased>()) {
			internal::OnKeyUp(key->code);
		}
		else if (const auto* button = event.getIf<sf::Event::MouseButtonPressed>()) {
			internal::OnMouseDown(button->button);
		}
		else if (const auto* button = event.getIf<sf::Event::MouseButtonReleased>()) {
			internal::OnMouseUp(button->button);
		}
		else if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
			internal::SetMousePosition(move->position);
		}
		else if (event.is<sf::Event::FocusLost>()) {
			internal::ClearAll();
		}

		return false; // never consumes
	}
}
