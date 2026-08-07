#pragma once
#include <cstdint>
#include <string_view>

namespace game::entities {
	// FNV-1a constexpr so it can produce enum values
	constexpr uint64_t hash(std::string_view str) {
		uint64_t hash = 14695981039346656037ull;

		for (char c : str) {
			// cast to uint8 makes it so it doesnt poison the hash with negative sign
			hash ^= static_cast<uint64_t>(static_cast<uint8_t>(c));
			hash *= 1099511628211ull;
		}

		return hash;
	}

	// every value must be the same as its LDTK identifier hash("<LDTK entity identifier>")
	enum class EntityType : uint64_t {
		Player = hash("Player"),
		Dragon = hash("Dragon"),
	};

	constexpr EntityType FromLdtkIdentifier(std::string_view identifier) {
		return static_cast<EntityType>(hash(identifier));
	}
}
