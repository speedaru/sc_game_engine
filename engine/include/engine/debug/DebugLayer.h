#pragma once
#include <engine/debug/debug_config.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <SFML/Window/Keyboard.hpp>

#include <engine/core/ILayer.h>
#include <engine/debug/DebugDraw.h>

namespace sc::debug {
	// drives the debug subsystem. Application pushes this on top of every game layer in
	// debug builds, so it gets first refusal on events and renders its overlay last
	class DebugLayer : public core::ILayer {
	public:
		static constexpr sf::Keyboard::Key TOGGLE_KEY = sf::Keyboard::Key::F1;

		void OnDetach() override;
		bool OnEvent(const sf::Event& event) override;
		void OnUpdate(float deltaTime) override;
		void OnRender(sf::RenderWindow& window) override;

	private:
		DebugDraw m_draw;
	};
}

#endif
