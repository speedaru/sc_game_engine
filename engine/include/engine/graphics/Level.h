#pragma once
#include <vector>
#include <memory>

#include <engine/graphics/TileMap.h>

namespace sc::graphics {
    class Level {
    public:
        Level(sf::Vector2i levelSize) : m_size(levelSize) {}

        void AddLayer(const std::shared_ptr<TileMap>& layer);

		const std::vector<std::shared_ptr<TileMap>>& GetLayers() const { return m_layers; }
		sf::Vector2i GetSize() const { return m_size; }

    private:
        // stored in bottom to top rendering order
        std::vector<std::shared_ptr<TileMap>> m_layers;
        sf::Vector2i m_size;
    };
}