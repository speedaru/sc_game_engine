#include <pch.h>
#include <loaders/world/HitboxUtils.h>

#include <engine/utils/logging.h>

namespace ecs = sc::ecs;

namespace game::world_loader {
	std::vector<ecs::Hitbox> ParseHitboxes(const nlohmann::json& hitboxArray, sf::Vector2f origin) {
		std::vector<ecs::Hitbox> hitboxes;

		if (!hitboxArray.is_array()) {
			LOG_W("hitbox custom data was not a json array, ignoring it");
			return hitboxes;
		}

		hitboxes.reserve(hitboxArray.size());

		for (const auto& box : hitboxArray) {
			hitboxes.emplace_back(
				box.value("x", 0.f) - origin.x,
				box.value("y", 0.f) - origin.y,
				box.value("w", 16.f),
				box.value("h", 16.f)
			);
		}

		return hitboxes;
	}

	std::vector<ecs::Hitbox> ParseHitboxesFromCustomData(const std::string& jsonStr, sf::Vector2f origin) {
		if (jsonStr.empty()) return {};

		nlohmann::json data;
		try {
			data = nlohmann::json::parse(jsonStr);
		}
		catch (const nlohmann::json::parse_error& e) {
			LOG_E("failed to parse custom data json: %s", e.what());
			return {};
		}

		if (!data.is_object()) {
			LOG_E("custom data must be a json object of named fields (e.g. {\"hitboxes\": [...]})");
			return {};
		}

		auto it = data.find("hitboxes");
		if (it == data.end()) return {};

		return ParseHitboxes(*it, origin);
	}

	int GetTileIdFromRect(const ldtk::IntRect& rect, const ldtk::Tileset& tileset) {
		const int stride = tileset.tile_size + tileset.spacing;
		const int columns = tileset.texture_size.x / tileset.tile_size;

		return ((rect.y - tileset.padding) / stride) * columns +
			((rect.x - tileset.padding) / stride);
	}
}
