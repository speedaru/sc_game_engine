#pragma once
#include <vector>

#include <nlohmann/json.hpp>

namespace sc_editor {

	// mirrors the "hitboxes" entry of the engine's custom-data schema
	// (see very_cool_rpg/src/loaders/world/CustomData.h)
	struct Hitbox {
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
	};

	void to_json(nlohmann::json& j, const Hitbox& hb);
	void from_json(const nlohmann::json& j, Hitbox& hb);

	// the root shape of an LDtk tile/entity "customData" string, e.g. {"hitboxes": [...]}
	// this is the single place that knows that shape - add a new field here (plus its
	// to_json/from_json handling below) and the rest of the editor doesn't need to change,
	// as long as it isn't editing that field yet.
	struct CustomData {
		std::vector<Hitbox> hitboxes;

		// any top-level fields this editor doesn't have UI for (e.g. authored by the game
		// or a future editor feature). Kept around so saving never destroys them.
		nlohmann::json extra = nlohmann::json::object();

		bool IsEmpty() const { return hitboxes.empty() && extra.empty(); }
	};

	void to_json(nlohmann::json& j, const CustomData& data);
	void from_json(const nlohmann::json& j, CustomData& data);

}
