#pragma once
#include <engine/world/TileLayer.h>

namespace sc::world {
    class YSortedTileLayer : public ILevelLayer {
    public:
        YSortedTileLayer(const LayerId& id, std::shared_ptr<graphics::TileSet> tileSet)
            : ILevelLayer(LayerType::YSortedTile, id, false),
            m_tileSet(std::move(tileSet)) {}

        void LoadTiles(const std::vector<TileInstance>& tiles) {
            m_tiles = tiles;
        }

        const std::vector<TileInstance>& GetTiles() const { return m_tiles; }
        const std::shared_ptr<sc::graphics::TileSet>& GetTileSet() const { return m_tileSet; }

    private:
        std::shared_ptr<sc::graphics::TileSet> m_tileSet;
        std::vector<TileInstance> m_tiles;
    };
}
