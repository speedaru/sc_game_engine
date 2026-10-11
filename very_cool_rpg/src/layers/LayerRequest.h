#pragma once
#include <cstdint>
#include <variant>

#include <SFML/System/Vector2.hpp>

namespace game {
	// handlers can't switch level while the contacts are being iterated, so they record
	// the request and the owner of the state applies it after DispatchContacts returns
	struct TeleportRequest {
		int32_t levelUid;
		sf::Vector2f pos;
	};

	using LayerRequest = std::variant<TeleportRequest>;
}
