#pragma once
#include <cstdint>
#include <string>

#include <SFML/System/Vector2.hpp>

#include <entities/EntityField.h>

namespace game::entities {
	// per entity instance spawn data
	struct SpawnParams {
		sf::Vector2f position;

		// EntityLayer in which this entity belongs to, render_system requires layerUid to properly handle layers
		// -1 by default is an invalid layer, meaning its never drawn if no valid uid is specified
		int32_t layerUid = -1;

		// the same on every run for an entity placed in the editor (its ldtk iid), so a save file
		// can refer to it. empty for an entity spawned at runtime
		std::string persistentId;

		// TODO: per instance collider overrides std::optional<std::vector<sc::math::Hitbox>>
		FieldBag fields;
	};
}
