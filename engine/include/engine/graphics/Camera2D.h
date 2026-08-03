#pragma once
#include <SFML/Graphics/View.hpp>

namespace sc::graphics {
	struct CameraBounds {
		sf::Vector2f min;
		sf::Vector2f max;
	};

	class Camera2D {
	public:
		Camera2D(float width, float height) : Camera2D(sf::Vector2f(width, height)) {}
		Camera2D(const sf::Vector2f size);
		
		void SetPosition(const sf::Vector2f& position);
		void SetZoom(float zoomFactor);

		// min and max refer to visible borders positions not camera position delimiters
		void SetBounds(const sf::Vector2f& min, const sf::Vector2f& max);

		const sf::View& GetView() const { return m_view; }
		const sf::Vector2f GetSize() const { return m_view.getSize(); }
		const sf::Vector2f GetHalfSize() const { return m_view.getSize() / 2.f; }

	private:
		// checks if bounds is set before applying bounds. returns camera position within bounds
		sf::Vector2f ApplyBounds(sf::Vector2f position);

	private:
		std::optional<CameraBounds> m_bounds;
		sf::View m_view;
	};
}
