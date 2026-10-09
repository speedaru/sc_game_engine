#include <pch.h>
#include <debug/ContactOverlay.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <imgui.h>

#include <engine/debug/DebugDraw.h>
#include <engine/physics/Collisions.h>

namespace game::debug {
	ContactOverlay::ContactOverlay(const sc::physics::PhysicsScratch& scratch)
		: m_scratch(scratch) {}

	void ContactOverlay::OnDrawUI(const DebugContext& ctx) {
		ImGui::Text("%zu contacts in the last physics step", m_scratch.contacts.size());
		ImGui::SetItemTooltip("a mover can have up to one contact per slide, so a corner reports two");

		ImGui::Checkbox("Hit colliders", &m_showColliders);
		ImGui::SetItemTooltip("orange is a tile, pink is an entity");
		ImGui::Checkbox("Normals", &m_showNormals);
		ImGui::SetItemTooltip("drawn from the face that was hit, pointing back at the mover");
	}

	void ContactOverlay::OnDrawOverlay(const DebugContext& ctx, DebugDraw& draw) {
		for (const sc::physics::Contact& contact : m_scratch.contacts) {
			const sf::FloatRect& box = contact.other.box;

			const bool isOverlap = contact.kind == sc::physics::ContactKind::Overlap;
			if (m_showColliders) {
				const bool isTile = contact.other.type == sc::physics::ColliderType::Tile;
				sf::Color color = isTile ? TILE_COLOR : ENTITY_COLOR;
				if (isOverlap) color = OVERLAP_COLOR;

				draw.Rect(box, color);
			}

			if (isOverlap) continue; // dont drow arrows for overlapping boxes

			if (m_showNormals) {
				// the normal points away from the hit face
				const sf::Vector2f center = box.position + box.size / 2.f;
				const sf::Vector2f faceCenter{
					center.x + contact.normal.x * box.size.x / 2.f,
					center.y + contact.normal.y * box.size.y / 2.f
				};

				draw.Arrow(faceCenter, faceCenter + contact.normal * NORMAL_LENGTH, NORMAL_COLOR);
			}
		}
	}
}

#endif /* SC_ENABLE_DEBUG_TOOLS */
