#pragma once
#include <string>
#include <vector>

#include <SFML/System/Vector2.hpp>
#include <LDtkLoader/Tileset.hpp>
#include <nlohmann/json.hpp>

#include <engine/ecs/Components.h>

namespace game::world_loader {
	// parses a JSON array of {x, y, w, h} rects into hitboxes
	std::vector<sc::ecs::Hitbox> ParseHitboxes(const nlohmann::json& hitboxArray, sf::Vector2f origin = { 0.f, 0.f });

	// extract hitboxes from root JSON custom data
	std::vector<sc::ecs::Hitbox> ParseHitboxesFromCustomData(const std::string& jsonStr, sf::Vector2f origin = { 0.f, 0.f });

	// find tile id from a texture rect inside a ldtk tileset
	int GetTileIdFromRect(const ldtk::IntRect& rect, const ldtk::Tileset& tileset);
}
