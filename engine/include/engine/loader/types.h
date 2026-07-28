#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

#include <SFML/Graphics.hpp>

#include <engine/graphics/Texture2D.h>

namespace sc::loader {
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
}
