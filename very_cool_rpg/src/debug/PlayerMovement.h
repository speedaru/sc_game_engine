#pragma once
#include <engine/debug/debug_config.h>

namespace game::debug {}

#if SC_ENABLE_DEBUG_TOOLS
#include <SFML/Graphics/Color.hpp>

#include <engine/debug/IDebugModule.h>

namespace game::debug {
	using namespace sc::debug;

	class PlayerMovement : public IDebugModule {
	public:
		const char* Name() const override { return "Movement"; }
		const char* Category() const override { return "Physics"; }

		void OnDrawUI(const DebugContext& ctx) override;
		void OnDrawOverlay(const DebugContext& ctx, DebugDraw& draw) override;

	private:
		void DrawMovementeVec(const DebugContext& ctx, DebugDraw& draw);

	private:
		static constexpr sf::Color VEL_VEC_ARROW{ 0, 255, 255, 205 };

		bool m_showMovementVec;
	};
}

#endif
