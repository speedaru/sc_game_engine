#include <game/utils/render_utils.h>

#include <engine/ecs/Components.h>
#include <engine/graphics/render2d.h>

namespace game::render_utils {
	sc::graphics::QuadProps CreateQuadProps(const sc::ecs::Entity& entity) {
		sc::graphics::QuadProps props;

		auto& transform = entity.GetComponent<sc::ecs::TransformComponent>();
		auto& sprite = entity.GetComponent<sc::ecs::SpriteComponent>();

		props.position = transform.pos;
		props.texture = sprite.texture;

		// game specific components

		return props;
	}
}
