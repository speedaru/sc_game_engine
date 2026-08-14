#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

namespace sc::input {
	using Key = sf::Keyboard::Key;
	using MouseButton = sf::Mouse::Button;

	bool IsKeyDown(Key key);
	bool IsKeyPressed(Key key);
	bool IsKeyReleased(Key key);

	bool IsMouseButtonDown(MouseButton button);
	bool IsMouseButtonPressed(MouseButton button);
	bool IsMouseButtonReleased(MouseButton button);

	sf::Vector2i GetMousePosition();

	// clear previous pressed and released keys
	void NewFrame();

	// written only by InputLayer
	namespace internal {
		void OnKeyDown(Key key);
		void OnKeyUp(Key key);
		void OnMouseDown(MouseButton button);
		void OnMouseUp(MouseButton button);
		void SetMousePosition(sf::Vector2i pos);

		// reset every input state to up, used so when window loses focus, keys dont stay down
		void ClearAll();
	}
}
