#pragma once
#include <string>
#include <functional>

#include <SFML/System/Vector2.hpp>
#include <nlohmann/json.hpp>

namespace sc::ecs { class Entity; }

namespace game::world_loader {
	// receives the value of a single custom-data field (e.g. the array under "hitboxes")
	// `originOffset` is only meaningful for entity custom data (aligns to the entity's pivot); tile
	// custom data passes {0, 0}
	using CustomDataHandler = std::function<void(const sc::ecs::Entity& entity, const nlohmann::json& value, sf::Vector2f originOffset)>;

	// registers a handler for a top-level key in the custom-data JSON object, e.g. RegisterCustomDataField("hitboxes", ...)
	// call once per field, from whichever file owns that feature
	void RegisterCustomDataField(const std::string& key, CustomDataHandler handler);

	// registers the engine's built-in fields (currently just "hitboxes")
	void RegisterBuiltinCustomDataFields();

	// parses `jsonStr` as a JSON object and dispatches each present key to its registered handler
	// custom-data strings are authored per-tile/per-entity in LDtk; expected shape is
	// { "hitboxes": [...], ... } - unregistered keys are logged and skipped, not an error
	void ApplyCustomData(const sc::ecs::Entity& entity, const std::string& jsonStr, sf::Vector2f originOffset = { 0.f, 0.f });
}
