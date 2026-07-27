#pragma once
#include <string>
#include <cstdint>

#include <SFML/Graphics.hpp>

namespace sc::core {
	struct WindowData {
		std::string title;
		uint32_t width;
		uint32_t height;
	};

	class Window {
	public:
		Window(WindowData&& windowData);
		~Window() {}

		void Update();

		sf::RenderWindow& GetNativeWindow() { return m_window; }

	private:
		sf::RenderWindow m_window;
		WindowData m_data;
	};
}
