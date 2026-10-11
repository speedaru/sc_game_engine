#include <pch.h>
#include <engine/constants.h>
#include <engine/core/Window.h>

namespace sc::core {
	Window::Window(WindowData&& windowData) {
		m_window.create(sf::VideoMode(sf::Vector2u(windowData.width, windowData.height)), windowData.title);
		m_window.setFramerateLimit(MAX_FPS);
	}
}

