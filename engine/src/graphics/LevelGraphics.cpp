#include <pch.h>
#include <engine/graphics/LevelGraphics.h>

namespace sc::graphics {
	void LevelGraphics::AddLayer(std::shared_ptr<TileMap> layer) {
		m_layers.push_back(layer);
	}
}