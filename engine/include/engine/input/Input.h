#pragma once
#include <memory>
#include <type_traits>

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

	// same as above for a game's own action enum, so call sites don't cast

	template <typename Action> requires std::is_enum_v<Action>
	bool IsActionActive(Action action) { return IsActionActive(static_cast<ActionId>(action)); }

	template <typename Action> requires std::is_enum_v<Action>
	bool IsActionJustPressed(Action action) { return IsActionJustPressed(static_cast<ActionId>(action)); }

	template <typename Action> requires std::is_enum_v<Action>
	bool IsActionJustReleased(Action action) { return IsActionJustReleased(static_cast<ActionId>(action)); }
}
