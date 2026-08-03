#pragma once
#include <string>
#include <vector>
#include <map>
#include <utility>

#include <nlohmann/json.hpp>

#include "CustomData.h"

namespace sc_editor {

	struct TilesetInfo {
		std::string identifier;
		std::string relPath;
		int columns = 0;   // __cWid
		int rows = 0;      // __cHei
		int tileSize = 0;  // tileGridSize

		// Points directly at this tileset's entry in LDtkEditorData::m_root, captured once
		// at load time. Avoids ever re-deriving an index into the JSON array, which is what
		// caused Internal_Icons being skipped to desync tileset indices from JSON indices.
		nlohmann::json* defJson = nullptr;
	};

	// Owns the raw LDtk JSON document and lets the editor read/write tile
	// customData directly, since LDtkLoader only exposes a read-only view.
	class LDtkEditorData {
	public:
		bool Load(const std::string& path);

		const std::string& GetProjectPath() const { return m_projectPath; }
		const std::vector<TilesetInfo>& GetTilesets() const { return m_tilesets; }

		// also caches any custom-data fields this editor doesn't have UI for (see CustomData.h),
		// so a later SaveTileCollisions() call doesn't clobber them
		std::vector<Hitbox> GetTileCollisions(int tilesetIndex, int tileId);
		void SaveTileCollisions(int tilesetIndex, int tileId, const std::vector<Hitbox>& hitboxes);

	private:
		TilesetInfo* FindTilesetDef(int tilesetIndex);
		const TilesetInfo* FindTilesetDef(int tilesetIndex) const;

		nlohmann::json m_root;
		std::string m_projectPath;
		std::vector<TilesetInfo> m_tilesets;

		// custom-data fields this editor doesn't understand yet, keyed by (tilesetIndex, tileId)
		std::map<std::pair<int, int>, nlohmann::json> m_extraFieldsByTile;
	};

}
