#pragma once
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

#include <SFML/System/Vector2.hpp>

#include <engine/graphics/TileMap.h>
#include <engine/graphics/LevelGraphics.h>
namespace fs = std::filesystem;

namespace sc::loader {
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
