#pragma once
#include <memory>
#include <string>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <engine/ecs/Components.h>
#include <engine/graphics/Texture2D.h>

#include <entities/EntityType.h>

namespace game::entities {
	// entity definition for each entity TYPE, not for each entity INSTANCE
	// no ldtk types, EntityDefinitionLoader is the only thing that knows how to fill this
	// everything else can spawn entities without ldtk data
	struct EntityDefinition {
		EntityType type{};
		std::string debugName;

		std::shared_ptr<sc::graphics::Texture2D> texture;
		sf::IntRect textureRect;
		sf::Vector2f pivot;

		// already pivot adjusted
		std::vector<sc::ecs::Hitbox> hitboxes;
	};
}
