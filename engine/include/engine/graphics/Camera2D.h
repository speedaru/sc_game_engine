#pragma once
#include <SFML/Graphics/View.hpp>

namespace sc::graphics {
	class Camera2D {
	public:
		Camera2D(float width, float height) : Camera2D(sf::Vector2f(width, height)) {}
		Camera2D(const sf::Vector2f size);
		
		void SetPosition(const sf::Vector2f& position);

		void setZoom(float zoomFactor);

		const sf::View& GetView() const { return m_view; }

	private:
		sf::View m_view;
	};
}
