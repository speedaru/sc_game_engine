#pragma once
#include <enums/hash.hpp>

namespace game::entities {
	// every value must be the same as its LDTK identifier hash("<LDTK entity identifier>")
	enum class EntityType : uint64_t {
		Player = enums::hash("Player"),
		Dragon = enums::hash("Dragon"),
	};

	constexpr EntityType FromLdtkIdentifier(std::string_view identifier) {
		return static_cast<EntityType>(enums::hash(identifier));
	}
}
