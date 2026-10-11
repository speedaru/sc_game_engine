#pragma once
#include <variant>

#include <types/LevelLocation.h>

namespace game {
	// handlers can't switch level while the contacts are being iterated, so they record
	// the request and the owner of the queue applies it after DispatchContacts returns
	struct TeleportRequest {
		LevelLocation target;
	};

	using LayerRequest = std::variant<TeleportRequest>;
}
