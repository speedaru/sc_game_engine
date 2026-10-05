#pragma once
#include <memory>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>

#include <engine/graphics/TileSet.h>
#include <engine/world/ILevelLayer.h>

namespace sc::world {
	struct TileInstance {
        sf::Vector2f pixelPos;
        sf::IntRect textureRect;
    };

    class TileLayer : public ILevelLayer {
    public:
        TileLayer(const LayerId& id, std::shared_ptr<graphics::TileSet> tileSet)
            : ILevelLayer(LayerType::Tile, id, true),
            m_tileSet(std::move(tileSet)) {}

        // populates VertexArray from data instances
        void LoadTiles(const std::vector<TileInstance>& tiles);

        const sf::VertexArray& GetVertices() const { return m_vertices; }
        const std::shared_ptr<sc::graphics::TileSet>& GetTileSet() const { return m_tileSet; }

    private:
        static constexpr size_t TILE_VERTICE_COUNT = 6; // number of vertices per tile
        std::shared_ptr<sc::graphics::TileSet> m_tileSet;
        sf::VertexArray m_vertices;
    };
}
