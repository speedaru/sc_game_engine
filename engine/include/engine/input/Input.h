#pragma once
#include <vector>
#include <memory>

#include <engine/input/InputContext.h>

namespace sc::input {
	void PushContext(const std::shared_ptr<InputContext>& ctx);

	void PopContext();

	void ClearContexts();

	bool IsActionActive(ActionId action);
}
