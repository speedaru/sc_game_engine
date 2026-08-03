#include "CustomData.h"

namespace sc_editor {

	void to_json(nlohmann::json& j, const Hitbox& hb) {
		j = { {"x", hb.x}, {"y", hb.y}, {"w", hb.w}, {"h", hb.h} };
	}

	void from_json(const nlohmann::json& j, Hitbox& hb) {
		hb.x = j.value("x", 0);
		hb.y = j.value("y", 0);
		hb.w = j.value("w", 0);
		hb.h = j.value("h", 0);
	}

	void to_json(nlohmann::json& j, const CustomData& data) {
		j = data.extra.is_object() ? data.extra : nlohmann::json::object();

		if (!data.hitboxes.empty()) {
			j["hitboxes"] = data.hitboxes;
		}
		else {
			j.erase("hitboxes");
		}
	}

	void from_json(const nlohmann::json& j, CustomData& data) {
		data = CustomData{};

		// legacy format: a bare array of hitboxes with no wrapping object. Read-only migration
		// path - anything saved through this editor is written back out in the new format.
		if (j.is_array()) {
			data.hitboxes = j.get<std::vector<Hitbox>>();
			return;
		}

		if (!j.is_object()) {
			return;
		}

		data.extra = j;
		if (j.contains("hitboxes")) {
			data.hitboxes = j.at("hitboxes").get<std::vector<Hitbox>>();
			data.extra.erase("hitboxes");
		}
	}

}
