#include <pch.h>
#include <engine/graphics/Camera2D.h>

namespace sc::graphics {
	Camera2D::Camera2D(const sf::Vector2f size) {
		m_view.setSize(size);
		m_view.setCenter(size / 2.f);
	}
	
	void Camera2D::SetPosition(const sf::Vector2f& position) {
		m_view.setCenter(ApplyBounds(position));
	}

	void Camera2D::SetZoom(float zoomFactor) {
		m_view.zoom(zoomFactor);
	}

	void Camera2D::SetBounds(const sf::Vector2f& min, const sf::Vector2f& max) {
		m_bounds = CameraBounds(min, max);
	}

	sf::Vector2f Camera2D::ApplyBounds(sf::Vector2f position) {
		if (!m_bounds.has_value())
			return position;

		const auto& bounds = m_bounds.value();
		auto camSize = m_view.getSize();

		// if bounds are smaller than camera size, center camera (per axis check)
		bool centerX = camSize.x > bounds.max.x - bounds.min.x;
		bool centerY = camSize.y > bounds.max.y - bounds.min.y;
		if (centerX || centerY) {
			if (centerX) {
				position.x = (bounds.max.x - bounds.min.x) / 2;
			}
			if (centerY) {
				position.y = (bounds.max.y - bounds.min.y) / 2;
			}
			return position;
		}

		// otherwise clamp edges
		position.x = std::clamp(position.x, bounds.min.x + camSize.x / 2, bounds.max.x - camSize.x / 2);
		position.y = std::clamp(position.y, bounds.min.y + camSize.y / 2, bounds.max.y - camSize.y / 2);

		return position;
	}
}