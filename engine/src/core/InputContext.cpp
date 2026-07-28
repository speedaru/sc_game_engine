#include <pch.h>
#include <engine/core/InputContext.h>

namespace sc::core {
	void InputContext::Bind(sf::Keyboard::Key key, ActionId action) {
		m_keyBindings[key] = action;
	}

	bool InputContext::IsActionPressed(ActionId action) const {
		// only check key press for this action
		for (const auto& [key, mappedAction] : m_keyBindings) {
			if (mappedAction == action && sf::Keyboard::isKeyPressed(key)) {
				return true;
			}
		}
		return false;
	}
}
