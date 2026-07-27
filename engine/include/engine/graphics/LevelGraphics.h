#pragma once
#include <vector>
#include <memory>

#include <engine/graphics/TileMap.h>

namespace sc::graphics {
    class LevelGraphics {
    public:
        void AddLayer(std::shared_ptr<TileMap> layer);

		const std::vector<std::shared_ptr<TileMap>>& GetLayers() const {
			return m_layers;
		}

    private:
        // stored in bottom to top rendering order
        std::vector<std::shared_ptr<TileMap>> m_layers;
    };
}