#include <pch.h>
#include <nlohmann/json.hpp>

#include <engine/loader/LDtkLoader.h>
#include <engine/utils/logging.h> // Assuming you have a logger

using json = nlohmann::json;

namespace sc::loader {
#pragma region private_helpers
    struct LDtkLoader::Data {
        json root;
        fs::path projectDir; // to resolve relative paths

        // cache tilesets by LDtk uid
        std::unordered_map<int, std::shared_ptr<graphics::TileSet>> cachedTileSets;

        // find a specific level by its identifier string
        const json* FindLevel(const std::string& levelName) const {
            if (!root.contains("levels")) return nullptr;

            for (const auto& level : root["levels"]) {
                if (level["identifier"] == levelName) {
                    return &level;
                }
            }
            return nullptr;
        }

        // find a specific layer inside a level
        const json* FindLayer(const json& level, const std::string& layerName) const {
            if (!level.contains("layerInstances")) return nullptr;

            for (const auto& layer : level["layerInstances"]) {
                if (layer["__identifier"] == layerName) {
                    return &layer;
                }
            }
            return nullptr;
        }

        // get or create tileset from json defs
        std::shared_ptr<sc::graphics::TileSet> GetOrCreateTileSet(int uid) {
            // if exists use cached
            if (cachedTileSets.find(uid) != cachedTileSets.end()) {
                return cachedTileSets[uid];
            }

            // find in json defs block
            if (!root.contains("defs") || !root["defs"].contains("tilesets")) {
                LOG_W("failed to find defs or defs::tilesets block in ldtk file");
                return nullptr;
            }

            for (const auto& tsDef : root["defs"]["tilesets"]) {
                if (tsDef["uid"].get<int>() != uid) {
                    continue;
                }

				std::string relPath = tsDef["relPath"].get<std::string>();
				uint32_t gridSize = tsDef["tileGridSize"].get<uint32_t>();

				// resolve full path
				fs::path fullPath = projectDir / relPath;

				// create and cache tileset
				auto tileset = std::make_shared<sc::graphics::TileSet>(fullPath, gridSize);
				cachedTileSets[uid] = tileset;
				return tileset;
			}

            return nullptr;
        }

        // extract tile data into our LayerData format
        void ExtractTileData(const json& layer, LayerData& outData) const {
            outData.gridWidth = layer["__cWid"].get<uint32_t>();
            outData.gridHeight = layer["__cHei"].get<uint32_t>();
            outData.tileSize = layer["__gridSize"].get<uint32_t>();

            // fill the array with -1 (empty tile)
            outData.tiles.assign(outData.gridWidth * outData.gridHeight, -1);

            // LDtk stores Tiles layers in 'gridTiles'
            if (layer.contains("gridTiles")) {
                for (const auto& tile : layer["gridTiles"]) {
                    uint32_t tileID = tile["t"].get<uint32_t>();
                    
                    // LDtk gives us the raw pixel coordinates of the tile
                    int pxX = tile["px"][0].get<int>();
                    int pxY = tile["px"][1].get<int>();

                    // convert pixel coordinates back to grid coordinates
                    int gridX = pxX / outData.tileSize;
                    int gridY = pxY / outData.tileSize;
                    int index = (gridY * outData.gridWidth) + gridX;

                    // safety bounds check
                    if (index >= 0 && index < outData.tiles.size()) {
                        outData.tiles[index] = tileID;
                    }
                }
            }
            // TODO: add IntGrid handling here
        }

        void ExtractEntityData(const json& entityLayer, std::vector<EntitySpawnData>& outEntities) {
            if (!entityLayer.contains("entityInstances")) {
                LOG_W("entity layer doesn't contain 'entityInstances' field");
                return;
            }

            // iterate through all entities in the layer
            for (const auto& entity : entityLayer["entityInstances"]) {
                EntitySpawnData spawn;

                // get name identifier
                spawn.identifier = entity["__identifier"].get<std::string>();

                // get position
                spawn.position.x = static_cast<float>(entity["px"][0].get<int>());
                spawn.position.y = static_cast<float>(entity["px"][1].get<int>());

                // get tileset texture and rect
                if (!entity["__tile"].is_null()) {
                    const auto& tileDef = entity["__tile"];
                    int tilesetUid = tileDef["tilesetUid"].get<int>();

                    // get rect
                    spawn.textureRect.position = { tileDef["x"].get<int>(), tileDef["y"].get<int>() };
                    spawn.textureRect.size = { tileDef["w"].get<int>(), tileDef["h"].get<int>() };

                    // get entity tileset
                    auto tileset = this->GetOrCreateTileSet(tilesetUid);
                    if (tileset) {
						spawn.tileSetTexture = tileset->GetTexture();
                    }
                }

                outEntities.push_back(spawn);
            }
        }
    };
#pragma endregion // private_helpers


	LDtkLoader::LDtkLoader() : m_data(std::make_unique<Data>()) {}
    LDtkLoader::~LDtkLoader() = default;

    bool LDtkLoader::LoadProject(const fs::path& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            LOG_E("LDtkLoader failed to open file: %s", filepath.string().c_str());
            return false;
        }

        try {
            file >> m_data->root;
            m_data->projectDir = fs::absolute(filepath).parent_path();
            m_data->cachedTileSets.clear(); // clear tilesets on new project load
            LOG_I("Successfully loaded LDtk project: %s", filepath.string().c_str());
            return true;
        }
        catch (const json::parse_error&) {
            LOG_E("LDtkLoader JSON Parsing Error: %s", filepath.string().c_str());
            return false;
        }
    }

	std::shared_ptr<graphics::LevelGraphics> LDtkLoader::LoadLevelGraphics(const std::string& levelName) {
		auto levelGraphics = std::make_shared<sc::graphics::LevelGraphics>();

		const json* level = m_data->FindLevel(levelName);
		if (!level || !level->contains("layerInstances")) return levelGraphics;

        // iterate backwards to render bottom to top (LDtk stores top to bottom)
		const auto& layers = (*level)["layerInstances"];
		for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
			const auto& layer = *it;

            // only process "Tiles" layers
            if (layer["__type"] != "Tiles") continue;

            // get tileset UID for this layer
			int tilesetUid = layer["__tilesetDefUid"].is_null() ? -1 : layer["__tilesetDefUid"].get<int>();
            if (tilesetUid == -1) continue;

			auto tileSet = m_data->GetOrCreateTileSet(tilesetUid);
			if (tileSet) {
				LayerData data;
				m_data->ExtractTileData(layer, data);

                // build TileMap
				auto tileMap = std::make_shared<sc::graphics::TileMap>(tileSet);
				tileMap->Load(data.tiles, data.gridWidth, data.gridHeight);

                // add tilemap to level object
				levelGraphics->AddLayer(tileMap);
			}
		}

		return levelGraphics;
	}

    LayerData LDtkLoader::GetLayer(const std::string& levelName, const std::string& layerName) const {
        LayerData result;

        // find level
        const json* level = m_data->FindLevel(levelName);
        if (!level) {
            LOG_E("LDtkLoader could not find level: {}", levelName);
            return result; 
        }

        // find layer
        const json* layer = m_data->FindLayer(*level, layerName);
        if (!layer) {
            LOG_E("LDtkLoader could not find layer '{}' in level '{}'", layerName, levelName);
            return result;
        }

        // extract tile data
        m_data->ExtractTileData(*layer, result);
        
        return result;
    }

    std::vector<EntitySpawnData> LDtkLoader::GetEntities(const std::string& levelName, const std::string& layerName) const {
        std::vector<EntitySpawnData> result;

        // 1. Find the Level
        const json* level = m_data->FindLevel(levelName);
        if (!level) {
            LOG_E("LDtkLoader could not find level: {}", levelName);
            return result;
        }

        // 2. Find the Layer
        const json* layer = m_data->FindLayer(*level, layerName);
        if (!layer) {
            LOG_E("LDtkLoader could not find layer '{}' in level '{}'", layerName, levelName);
            return result;
        }

        // 3. Extract the entities
        m_data->ExtractEntityData(*layer, result);

        return result;
    }

}
