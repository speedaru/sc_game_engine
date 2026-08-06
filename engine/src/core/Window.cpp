#include <pch.h>
#include <engine/constants.h>
#include <engine/core/Window.h>

namespace sc::core {
	Window::Window(WindowData&& windowData)
		: m_data(std::move(windowData))
	{
		m_window.create(sf::VideoMode(sf::Vector2u(m_data.width, m_data.height)), m_data.title);
		m_window.setFramerateLimit(MAX_FPS);
	}
}

