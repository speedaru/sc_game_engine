#include <pch.h>
#include <engine/graphics/Camera2D.h>

namespace sc::graphics {
	Camera2D::Camera2D(const sf::Vector2f size) {
		m_view.setSize(size);
		m_view.setCenter(size / 2.f);
	}
	
	void Camera2D::SetPosition(const sf::Vector2f& position) {
		m_view.setCenter(position);
	}

	void Camera2D::setZoom(float zoomFactor) {
		m_view.zoom(zoomFactor);
	}
}