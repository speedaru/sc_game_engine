#pragma once
#include <cstdint>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <entt/entity/entity.hpp>

namespace sc::physics {
	// no cell index. tiles carry one, entities do not
	inline constexpr uint32_t INVALID_TILE = ~0u;

	// level space collision box
	// uses world coordinates instead of cell coordinates, because different layers can have different cell sizes
	struct ColliderRef {
		// world space coordinates
		sf::FloatRect box;

		// entity null means this collider is a tile
		// tileIndex identifies the tile type
		entt::entity entity = entt::null;
		uint32_t tileIndex = INVALID_TILE;

		bool IsTile() const { return entity == entt::null; }
	};

	using CandidateBuffer = std::vector<ColliderRef>;
}
