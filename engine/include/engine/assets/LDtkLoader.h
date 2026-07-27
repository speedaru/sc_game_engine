#pragma once
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

#include <SFML/System/Vector2.hpp>

#include <engine/graphics/TileMap.h>
#include <engine/graphics/LevelGraphics.h>
namespace fs = std::filesystem;

namespace sc::assets {
    struct LayerData {
        std::vector<int32_t> tiles;
        uint32_t gridWidth;
        uint32_t gridHeight;
        uint32_t tileSize;
    };

    struct EntitySpawnData {
        std::string identifier;
        sf::Vector2f position;
        std::shared_ptr<graphics::Texture2D> tileSetTexture;
        sf::IntRect textureRect;
    };

    class LDtkLoader {
    public:
        LDtkLoader();
		~LDtkLoader();

        bool LoadProject(const fs::path& filepath);

        std::shared_ptr<sc::graphics::LevelGraphics> LoadLevelGraphics(const std::string& levelName);

        LayerData GetLayer(const std::string& levelName, const std::string& layerName) const;

        std::vector<EntitySpawnData> GetEntities(const std::string& levelName, const std::string& layerName) const;

    private:
        struct Data;
        std::unique_ptr<Data> m_data; 
    };
}
