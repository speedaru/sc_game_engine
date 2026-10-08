#pragma once
#include <engine/debug/debug_config.h>

namespace game::debug {}

#if SC_ENABLE_DEBUG_TOOLS
#include <SFML/Graphics/Color.hpp>

#include <engine/debug/IDebugModule.h>

namespace sc::world {
	class World;
}

namespace game::debug {
	using namespace sc::debug;

	class FeatureTest : public IDebugModule {
	public:
		FeatureTest(const sc::world::World& world, std::function<void(int32_t)> onTeleport);

		const char* Name() const override { return "testing"; }
		const char* Category() const override { return "Features"; }

		void OnDrawUI(const DebugContext& ctx) override;
		void OnDrawOverlay(const DebugContext& ctx, DebugDraw& draw) override {}

	private:
		void DrawMovementeVec(const DebugContext& ctx, DebugDraw& draw);

	private:
		struct LevelSelectable {
			const std::string_view levelName;
			const int32_t levelUid;
		};

		const std::function<void(int32_t)> m_onTeleport;
		std::string_view m_currentlySelected = "%Level%";
		int32_t m_currentlySelectedUid = -1;
		std::vector<LevelSelectable> m_levelItems;
	};
}

#endif /* SC_ENABLE_DEBUG_TOOLS */
