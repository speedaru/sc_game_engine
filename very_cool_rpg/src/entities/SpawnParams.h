#pragma once
#include <cstdint>

#include <SFML/System/Vector2.hpp>

namespace game::entities {
	// per entity instance spawn data
	struct SpawnParams {
		sf::Vector2f position;

		// EntityLayer in which this entity belongs to, render_system requires layerUid to properly handle layers
		// -1 by default is an invalid layer, meaning its never drawn if no valid uid is specified
		int32_t layerUid = -1;

		// TODO: per instance collider overrides std::optional<std::vector<sc::math::Hitbox>>
	};
}
