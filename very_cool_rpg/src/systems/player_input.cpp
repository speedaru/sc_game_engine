#include <pch.h>

#include <engine/input/Input.h>
#include <engine/ecs/Registry.h>
#include <engine/ecs/Components.h>

#include <components/GameComponents.h>
#include <enums/InputActions.h>
#include <systems/player_input.h>

namespace ecs = sc::ecs;
namespace input = sc::input;

namespace game::systems  {
	void UpdatePlayerInput(sc::ecs::Registry& registry) {
		auto view = registry.GetRegistry().view<components::PlayerTag, components::CharacterController>();

		for (auto [entity, controller] : view.each()) {
			//LOG_D("found entity with player tag and character controller: %s", tag.tag.c_str());
			controller.direction = { 0.f, 0.f };

			if (input::IsActionActive(static_cast<int32_t>(InputAction::MoveUp)))		controller.direction.y -= 1.f;
			if (input::IsActionActive(static_cast<int32_t>(InputAction::MoveDown)))		controller.direction.y += 1.f;
			if (input::IsActionActive(static_cast<int32_t>(InputAction::MoveRight)))	controller.direction.x += 1.f;
			if (input::IsActionActive(static_cast<int32_t>(InputAction::MoveLeft)))		controller.direction.x -= 1.f;
		}
	}
}
