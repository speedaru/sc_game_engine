#pragma once
#include <engine/core/ILayer.h>

namespace sc::input {
	// gets event and updates InputState
	class InputLayer : public core::ILayer {
	public:
		bool OnEvent(const sf::Event& event) override;
	};
}
