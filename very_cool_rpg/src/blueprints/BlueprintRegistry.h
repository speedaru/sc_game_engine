#pragma once

namespace sc::ecs {
	class EntityFactory;
}

namespace game::blueprints {
	void RegisterAll(sc::ecs::EntityFactory& factory);
}
