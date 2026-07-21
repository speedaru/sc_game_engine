#pragma once
#include <SFML/Graphics/RenderWindow.hpp>

namespace sc::core {
	class ILayer {
	public:
		virtual ~ILayer() = default;

		// called when the layer is added to the application
		virtual void OnAttach() {}

		// called when the layer is removed
		virtual void OnDetach() {}

		// called every physics tick
		virtual void OnFixedUpdate(float timeStep) {}

		// called every single frame
		virtual void OnUpdate(float deltaTime) {}

		// called during the render phase
		virtual void OnRender(sf::RenderWindow& window) {}
	};
}
