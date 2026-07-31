#pragma once
#include <engine/world/ILevelLayer.h>

namespace sc::world {
	class EntityLayer : public ILevelLayer {
	public:
		EntityLayer(const LayerId& id) : ILevelLayer(LayerType::Entity, id, false) {}

		// requires no data, it just acts as a marker to indicate that the renderer
		// should query ECS registry to render entities
	};
}
