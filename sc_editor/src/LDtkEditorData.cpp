#include "LDtkEditorData.h"

#include <fstream>
#include <iostream>

namespace sc_editor {

	bool LDtkEditorData::Load(const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			std::cerr << "[LDtkEditorData] failed to open " << path << "\n";
			return false;
		}

		try {
			file >> m_root;
		}
		catch (const nlohmann::json::parse_error& e) {
			std::cerr << "[LDtkEditorData] failed to parse " << path << ": " << e.what() << "\n";
			return false;
		}

		m_projectPath = path;
		m_tilesets.clear();

		if (!m_root.contains("defs") || !m_root["defs"].contains("tilesets")) {
			return false;
		}

		auto& tilesetDefs = m_root["defs"]["tilesets"];
		for (auto& tsDef : tilesetDefs) {
			TilesetInfo info;
			info.identifier = tsDef.value("identifier", std::string());
			// skip internal icon tileset
			if (info.identifier == "Internal_Icons") {
				continue;
			}

			info.relPath = tsDef.value("relPath", std::string());
			info.columns = tsDef.value("__cWid", 0);
			info.rows = tsDef.value("__cHei", 0);
			info.tileSize = tsDef.value("tileGridSize", 0);
			info.defJson = &tsDef;
			m_tilesets.push_back(std::move(info));
		}

		return true;
	}

	TilesetInfo* LDtkEditorData::FindTilesetDef(int tilesetIndex) {
		if (tilesetIndex < 0 || tilesetIndex >= (int)m_tilesets.size()) {
			return nullptr;
		}
		return &m_tilesets[tilesetIndex];
	}

	const TilesetInfo* LDtkEditorData::FindTilesetDef(int tilesetIndex) const {
		if (tilesetIndex < 0 || tilesetIndex >= (int)m_tilesets.size()) {
			return nullptr;
		}
		return &m_tilesets[tilesetIndex];
	}

	std::vector<Hitbox> LDtkEditorData::GetTileCollisions(int tilesetIndex, int tileId) const {
		std::vector<Hitbox> result;

		const TilesetInfo* tsInfo = FindTilesetDef(tilesetIndex);
		if (!tsInfo || !tsInfo->defJson) {
			return result;
		}

		const nlohmann::json& tsDef = *tsInfo->defJson;
		if (!tsDef.contains("customData")) {
			return result;
		}

		for (const auto& entry : tsDef["customData"]) {
			if (entry.value("tileId", -1) != tileId) {
				continue;
			}

			const std::string data = entry.value("data", std::string());
			if (data.empty()) {
				break;
			}

			try {
				nlohmann::json boxes = nlohmann::json::parse(data);
				for (const auto& box : boxes) {
					Hitbox hb;
					hb.x = box.value("x", 0);
					hb.y = box.value("y", 0);
					hb.w = box.value("w", 0);
					hb.h = box.value("h", 0);
					result.push_back(hb);
				}
			}
			catch (const nlohmann::json::parse_error& e) {
				std::cerr << "[LDtkEditorData] failed to parse customData for tile " << tileId << ": " << e.what() << "\n";
			}
			break;
		}

		return result;
	}

	void LDtkEditorData::SaveTileCollisions(int tilesetIndex, int tileId, const std::vector<Hitbox>& hitboxes) {
		TilesetInfo* tsInfo = FindTilesetDef(tilesetIndex);
		if (!tsInfo || !tsInfo->defJson) {
			std::cerr << "[LDtkEditorData] invalid tileset index " << tilesetIndex << "\n";
			return;
		}

		nlohmann::json& tsDef = *tsInfo->defJson;

		if (!tsDef.contains("customData") || !tsDef["customData"].is_array()) {
			tsDef["customData"] = nlohmann::json::array();
		}
		nlohmann::json& customData = tsDef["customData"];

		// drop any existing entry for this tile; re-added below if it still has boxes
		for (auto it = customData.begin(); it != customData.end(); ++it) {
			if (it->value("tileId", -1) == tileId) {
				customData.erase(it);
				break;
			}
		}

		if (!hitboxes.empty()) {
			nlohmann::json boxes = nlohmann::json::array();
			for (const auto& hb : hitboxes) {
				boxes.push_back({ {"x", hb.x}, {"y", hb.y}, {"w", hb.w}, {"h", hb.h} });
			}

			nlohmann::json entry;
			entry["tileId"] = tileId;
			entry["data"] = boxes.dump(2);
			customData.push_back(std::move(entry));
		}

		std::ofstream file(m_projectPath);
		if (!file.is_open()) {
			std::cerr << "[LDtkEditorData] failed to open " << m_projectPath << " for writing\n";
			return;
		}
		file << m_root.dump(1, '\t');

		std::cout << "[LDtkEditorData] saved collisions for tile " << tileId << " in tileset: " << tsDef["identifier"] << " to " << m_projectPath << "\n";
	}

}
