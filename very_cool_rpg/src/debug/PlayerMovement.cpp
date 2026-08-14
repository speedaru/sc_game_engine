#include <pch.h>
#include <debug/PlayerMovement.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <imgui.h>

#include <engine/debug/DebugDraw.h>
#include <engine/ecs/Registry.h>
#include <engine/ecs/Components.h>

#include <components/GameComponents.h>

namespace ecs = sc::ecs;

namespace game::debug {
	void PlayerMovement ::OnDrawUI(const DebugContext& ctx) {
		auto view = ctx.registry->GetRegistry().view<ecs::TransformComponent, ecs::VelocityComponent, components::PlayerTag>();

		for (auto [ent, trans, vel] : view.each()) {
			ImGui::Text("player pos: %.2f %.2f", trans.pos.x, trans.pos.y);
			ImGui::Text("player velocity: %.2f %.2f", vel.velocity.x, vel.velocity.y);
		}

		ImGui::Separator();

		ImGui::Checkbox("show velocity vector", &m_showMovementVec);
	}

	void PlayerMovement ::OnDrawOverlay(const DebugContext& ctx, DebugDraw& draw) {
		DrawMovementeVec(ctx, draw);
	}

	void PlayerMovement ::DrawMovementeVec(const DebugContext& ctx, DebugDraw& draw) {
		if (!m_showMovementVec) return;

		auto view = ctx.registry->GetRegistry().view<ecs::TransformComponent, ecs::VelocityComponent, components::PlayerTag>();

		for (auto [ent, trans, vel] : view.each()) {
			const auto& pos = trans.pos;
			const auto& velocity = vel.velocity;

			const sf::Vector2f size = { velocity.x / 4.f, velocity.y / 4.f };
			draw.Arrow(pos, pos + size, VEL_VEC_ARROW);
		}

	}
}

#endif