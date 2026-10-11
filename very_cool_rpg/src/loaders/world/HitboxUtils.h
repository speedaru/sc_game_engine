#pragma once
#include <string>
#include <vector>

#include <SFML/System/Vector2.hpp>
#include <LDtkLoader/Tileset.hpp>

#include <engine/math/Hitbox.h>

namespace game::world_loader {
	// extract hitboxes from root JSON custom data
	std::vector<sc::math::Hitbox> ParseHitboxesFromCustomData(const std::string& jsonStr, sf::Vector2f origin = { 0.f, 0.f });

	// find tile id from a texture rect inside a ldtk tileset
	int GetTileIdFromRect(const ldtk::IntRect& rect, const ldtk::Tileset& tileset);
}
