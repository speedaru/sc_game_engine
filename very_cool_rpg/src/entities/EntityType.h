#pragma once
#include <utils/hash.hpp>

namespace game::entities {
	// every value must be the same as its LDTK identifier hash("<LDTK entity identifier>")
	enum class EntityType : uint64_t {
		Player = utils::hash("Player"),
		Dragon = utils::hash("Dragon"),
		Chest = utils::hash("Chest"),
		TeleportationWell = utils::hash("TeleportationWell"),
	};

	constexpr EntityType FromLdtkIdentifier(std::string_view identifier) {
		return static_cast<EntityType>(utils::hash(identifier));
	}
}
