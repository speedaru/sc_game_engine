#pragma once
#include <engine/debug/debug_config.h>

namespace game::debug {}

#if SC_ENABLE_DEBUG_TOOLS
#include <SFML/Graphics/Color.hpp>

#include <engine/debug/IDebugModule.h>
#include <engine/physics/Collisions.h>

namespace game::debug {
	using namespace sc::debug;

	// draws the contacts the physics step reported, so what physics found can be checked by eye
	class ContactOverlay : public IDebugModule {
	public:
		explicit ContactOverlay(const sc::physics::PhysicsScratch& scratch);

		const char* Name() const override { return "Contacts"; }
		const char* Category() const override { return "Physics"; }

		void OnDrawUI(const DebugContext& ctx) override;
		void OnDrawOverlay(const DebugContext& ctx, DebugDraw& draw) override;

	private:
		static constexpr sf::Color TILE_COLOR{ 255, 165, 0, 220 };
		static constexpr sf::Color ENTITY_COLOR{ 255, 60, 200, 220 };
		static constexpr sf::Color NORMAL_COLOR{ 255, 255, 255, 230 };
		static constexpr float NORMAL_LENGTH = 12.f;

		const sc::physics::PhysicsScratch& m_scratch;

		bool m_showColliders = true;
		bool m_showNormals = true;
	};
}

#endif /* SC_ENABLE_DEBUG_TOOLS */
