#include <pch.h>
#include <engine/input/InputState.h>

#include <unordered_set>

namespace sc::input {
	namespace {
		// track input states (pressed, released, down) for mouse and keys
		template <typename T>
		class InputTracker {
		public:
			void OnDown(T value) {
				if (m_down.insert(value).second) {
					m_pressed.insert(value);
				}
			}

			void OnUp(T value) {
				if (m_down.erase(value)) {
					m_released.insert(value);
				}
			}

			void Clear() {
				m_down.clear();
			}

			void NewFrame() {
				m_pressed.clear();
				m_released.clear();
			}

			bool IsDown(T value) const { return m_down.count(value) > 0; }
			bool IsPressed(T value) const { return m_pressed.count(value) > 0; }
			bool IsReleased(T value) const { return m_released.count(value) > 0; }

		private:
			std::unordered_set<T> m_down;
			std::unordered_set<T> m_pressed;
			std::unordered_set<T> m_released;
		};

		InputTracker<Key> s_keys;
		InputTracker<MouseButton> s_mouseButtons;
		sf::Vector2i s_mousePos;
	}

	bool IsKeyDown(Key key) { return s_keys.IsDown(key); }
	bool IsKeyPressed(Key key) { return s_keys.IsPressed(key); }
	bool IsKeyReleased(Key key) { return s_keys.IsReleased(key); }

	bool IsMouseButtonDown(MouseButton button) { return s_mouseButtons.IsDown(button); }
	bool IsMouseButtonPressed(MouseButton button) { return s_mouseButtons.IsPressed(button); }
	bool IsMouseButtonReleased(MouseButton button) { return s_mouseButtons.IsReleased(button); }

	sf::Vector2i GetMousePosition() { return s_mousePos; }

	void NewFrame() {
		s_keys.NewFrame();
		s_mouseButtons.NewFrame();
	}

	namespace internal {
		void OnKeyDown(Key key) { s_keys.OnDown(key); }
		void OnKeyUp(Key key) { s_keys.OnUp(key); }
		void OnMouseDown(MouseButton button) { s_mouseButtons.OnDown(button); }
		void OnMouseUp(MouseButton button) { s_mouseButtons.OnUp(button); }
		void SetMousePosition(sf::Vector2i pos) { s_mousePos = pos; }

		void ClearAll() {
			s_keys.Clear();
			s_mouseButtons.Clear();
		}
	}
}
