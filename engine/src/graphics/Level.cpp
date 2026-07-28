#include <pch.h>
#include <engine/graphics/Level.h>

namespace sc::graphics {
	void Level::AddLayer(const std::shared_ptr<TileMap>& layer) {
		m_layers.push_back(layer);
	}
}