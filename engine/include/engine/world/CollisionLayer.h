#pragma once
#include <cstdint>
#include <vector>

#include <engine/world/ILevelLayer.h>

namespace sc::world {
    class CollisionLayer : public ILevelLayer {
    public:
        // width and height in tiles
        CollisionLayer(const LayerId& id, int width, int height, int gridSize)
            : ILevelLayer(LayerType::Collision, id, true),
            m_width(width), m_height(height), m_gridSize(gridSize)
        {
            m_grid.assign(m_width * m_height, EMPTY_TILE); // initialize empty grid
        }

        void SetData(const std::vector<int32_t>& gridData) { m_grid = gridData; }

        // helper function for the physics system to check overlaps
        bool IsSolid(float pixelX, float pixelY) const;

    private:
        static constexpr int32_t EMPTY_TILE = -1;
        int m_width;
        int m_height;
        int m_gridSize;
        std::vector<int32_t> m_grid;
    };
}
