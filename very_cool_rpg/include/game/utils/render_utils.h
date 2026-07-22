#pragma once
#include <engine/graphics/render2d.h>
#include <engine/ecs/Entity.h>

namespace game::render_utils {
	sc::graphics::QuadProps CreateQuadProps(const sc::ecs::Entity& entity);

	// submit an entity to 2d render engine and create a quad easier
	inline void SubmitEntity(const sc::ecs::Entity& entity) {
		sc::graphics::render2d::Submit(CreateQuadProps(entity));
	}
}
