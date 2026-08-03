#include <pch.h>
#include <loaders/world/CustomData.h>

#include <engine/ecs/Entity.h>
#include <engine/utils/logging.h>

#include <loaders/world/HitboxUtils.h>

using json = nlohmann::json;

namespace game::world_loader {
	namespace {
		std::unordered_map<std::string, CustomDataHandler>& GetFieldRegistry() {
			static std::unordered_map<std::string, CustomDataHandler> registry;
			return registry;
		}
	}

	void RegisterCustomDataField(const std::string& key, CustomDataHandler handler) {
		GetFieldRegistry()[key] = std::move(handler);
	}

	void RegisterBuiltinCustomDataFields() {
		RegisterCustomDataField("hitboxes", [](const sc::ecs::Entity& entity, const json& value, sf::Vector2f originOffset) {
			AddBoxCollider(entity, value, originOffset);
		});
	}

	void ApplyCustomData(const sc::ecs::Entity& entity, const std::string& jsonStr, sf::Vector2f originOffset) {
		json data;
		try {
			data = json::parse(jsonStr);
		}
		catch (const json::parse_error& e) {
			LOG_E("Failed to parse custom data JSON. Error: %s", e.what());
			return;
		}

		if (!data.is_object()) {
			LOG_E("Custom data must be a JSON object of named fields (e.g. {\"hitboxes\": [...]})");
			return;
		}

		const auto& registry = GetFieldRegistry();
		for (const auto& [key, value] : data.items()) {
			auto it = registry.find(key);
			if (it == registry.end()) {
				LOG_W("No custom data handler registered for field '%s', skipping", key.c_str());
				continue;
			}

			it->second(entity, value, originOffset);
		}
	}
}
