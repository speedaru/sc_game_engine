#include <pch.h>
#include <engine/input/InputContext.h>

#include <engine/input/InputState.h>
#include <engine/utils/logging.h>

namespace sc::input {
	void InputContext::Bind(Key key, ActionId action) {
		m_keyBindings[key] = action;
	}

	void InputContext::Bind(MouseButton button, ActionId action) {
		m_mouseBindings[button] = action;
	}

	bool InputContext::IsBound(ActionId action) const {
		// always return true bcs we just check if action is mapped
		return IsAction(action, [](auto) { return true; }, [](auto) { return true; });
	}

	bool InputContext::IsActionDown(ActionId action) const {
		return IsAction(action, IsKeyDown, IsMouseButtonDown);
	}

	bool InputContext::IsActionJustPressed(ActionId action) const {
		return IsAction(action, IsKeyPressed, IsMouseButtonPressed);
	}

	bool InputContext::IsActionJustReleased(ActionId action) const {
		return IsAction(action, IsKeyReleased, IsMouseButtonReleased);
	}

	__forceinline bool InputContext::IsAction(ActionId action, IsKey isKeyFn, IsMouse isMouseFn) const {
		for (const auto& [key, mappedAction] : m_keyBindings) {
			if (mappedAction == action && isKeyFn(key)) {
				return true;
			}
		}

		for (const auto& [mouse, mappedAction] : m_mouseBindings) {
			if (mappedAction == action && isMouseFn(mouse)) {
				return true;
			}
		}

		return false;
	}
}
