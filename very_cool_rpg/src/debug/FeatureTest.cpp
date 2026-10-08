#include <pch.h>
#include <debug/FeatureTest.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <imgui.h>

#include <engine/debug/DebugDraw.h>
#include <engine/world/World.h>
#include <engine/ecs/Registry.h>
#include <engine/ecs/Entity.h>
//#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/systems/lifecycle_system.h>
#include <engine/graphics/Camera2D.h>

#include <components/GameComponents.h>

namespace ecs = sc::ecs;

namespace game::debug {
	FeatureTest::FeatureTest(const sc::world::World& world, std::function<void(int32_t)> onTeleport)
		: m_onTeleport(onTeleport)
	{
		// get all levels
		for (const auto& [levelUid, level] : world.ViewLevels()) {
			m_levelItems.push_back(LevelSelectable(level->GetName(), level->GetUid()));
		}
	}

	void FeatureTest::OnDrawUI(const DebugContext& ctx) {
		if (ImGui::BeginCombo("teleport player to", m_currentlySelected.data())) {
			for (auto& selectable : m_levelItems) {
				if (ImGui::Selectable(selectable.levelName.data(), m_currentlySelectedUid == selectable.levelUid)) {
					m_currentlySelected = selectable.levelName;
					m_currentlySelectedUid = selectable.levelUid;
				}
			}


			ImGui::EndCombo();
		}

		if (ImGui::Button("Teleport")) {
			if (m_currentlySelectedUid == -1) {
				return;
			}

			m_onTeleport(m_currentlySelectedUid);
		}
	}
}

#endif /* SC_ENABLE_DEBUG_TOOLS */
