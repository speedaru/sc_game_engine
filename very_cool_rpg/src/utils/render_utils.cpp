#include <pch.h>
#include <utils/render_utils.h>

#include <engine/ecs/Entity.h>
#include <engine/ecs/Components.h>
#include <engine/graphics/render2d.h>

//#include <components/SpriteComponent.h>
namespace ecs = sc::ecs;
namespace gfx = sc::graphics;
//namespace gcomp = game::components;

namespace game::render_utils {
	gfx::QuadProps CreateQuadProps(const ecs::Entity& entity) {
		gfx::QuadProps props;

		// engine components
		auto& transform = entity.GetComponent<ecs::TransformComponent>();
		props.position = transform.pos;
		props.pivot = transform.pivot;

		// game components
		auto& sprite = entity.GetComponent<ecs::SpriteComponent>();
		props.texture = sprite.texture;
		props.textureRect = sprite.rect;
		props.zIndex = sprite.zlayer;

		return props;
	}
}
