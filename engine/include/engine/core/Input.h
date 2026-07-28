#pragma once
#include <vector>
#include <memory>

#include <engine/core/InputContext.h>

namespace sc::core {
	namespace input {
		void PushContext(const std::shared_ptr<InputContext>& ctx);

		void PopContext();

		void ClearContexts();

		bool IsActionActive(ActionId action);
	};
}
