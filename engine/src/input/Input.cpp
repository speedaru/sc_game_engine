#include <pch.h>
#include <engine/input/Input.h>

namespace sc::input {
	namespace {
		std::vector<std::shared_ptr<InputContext>> s_contexts;
	}

	void PushContext(const std::shared_ptr<InputContext>& ctx) {
		s_contexts.push_back(ctx);
	}

	void PopContext() {
		if (!s_contexts.empty()) {
			s_contexts.pop_back();
		}
	}

	void ClearContexts() {
		s_contexts.clear();
	}

	bool IsActionActive(ActionId action) {
		for (auto it = s_contexts.rbegin(); it != s_contexts.rend(); ++it) {
			if ((*it)->IsBound(action)) return (*it)->IsActionDown(action);
		}
		return false;
	}

	bool IsActionJustPressed(ActionId action) {
		for (auto it = s_contexts.rbegin(); it != s_contexts.rend(); ++it) {
			if ((*it)->IsBound(action)) return (*it)->IsActionJustPressed(action);
		}
		return false;
	}

	bool IsActionJustReleased(ActionId action) {
		for (auto it = s_contexts.rbegin(); it != s_contexts.rend(); ++it) {
			if ((*it)->IsBound(action)) return (*it)->IsActionJustReleased(action);
		}
		return false;
	}
}
