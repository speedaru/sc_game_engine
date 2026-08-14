#pragma once
#include <vector>
#include <memory>

#include <engine/input/InputContext.h>

namespace sc::input {
	void PushContext(const std::shared_ptr<InputContext>& ctx);

	void PopContext();

	void ClearContexts();

	// held state (while key held down)
	bool IsActionActive(ActionId action);

	// true exactly 1 per key press/release
	bool IsActionJustPressed(ActionId action);
	bool IsActionJustReleased(ActionId action);
}
