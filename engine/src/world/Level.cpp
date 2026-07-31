#include <pch.h>
#include <engine/world/Level.h>
#include <engine/utils/logging.h>

namespace sc::world {
	void Level::AddLayer(std::unique_ptr<ILevelLayer> layer) {
		if (layer) {
			m_layers.push_back(std::move(layer));
		}
	}
}
