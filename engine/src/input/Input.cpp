#include <pch.h>
#include <engine/input/Input.h>
#include <engine/utils/logging.h>

namespace sc::input {
	std::vector<std::shared_ptr<InputContext>> s_contexts;

	void input::PushContext(const std::shared_ptr<InputContext>& ctx) {
		s_contexts.push_back(ctx);
	}

	void input::PopContext() {
		if (!s_contexts.empty()) {
			s_contexts.pop_back();
		}
	}

	void input::ClearContexts() {
		s_contexts.clear();
	}

	bool input::IsActionActive(ActionId action) {
		if (s_contexts.empty()) {
			LOG_W("checking if action is active but no input context is active");
			return false;
		}

		return s_contexts.back()->IsActionPressed(action);
	}
}
