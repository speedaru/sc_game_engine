#include <pch.h>
#include <engine/debug/modules/CollisionOverlay.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <imgui.h>

#include <engine/graphics/Camera2D.h>
#include <engine/debug/DebugDraw.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/Registry.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/world/Level.h>

namespace sc::debug::modules {
	namespace {
		bool ColorEdit(const char* label, sf::Color& color) {
			float rgba[4] = {
				color.r / 255.f,
				color.g / 255.f,
				color.b / 255.f,
				color.a / 255.f
			};

			if (!ImGui::ColorEdit4(label, rgba, ImGuiColorEditFlags_AlphaBar)) return false;

			color = sf::Color(
				static_cast<std::uint8_t>(rgba[0] * 255.f),
				static_cast<std::uint8_t>(rgba[1] * 255.f),
				static_cast<std::uint8_t>(rgba[2] * 255.f),
				static_cast<std::uint8_t>(rgba[3] * 255.f)
			);
			return true;
		}
	}

	void CollisionOverlay::OnDrawUI(const DebugContext& ctx) {
		auto view = ctx.registry->GetRegistry().view<const ecs::TransformComponent, const ecs::BoxColliderComponent>();

		std::size_t colliderCount = 0;
		std::size_t hitboxCount = 0;
		for (auto [entity, trans, col] : view.each()) {
			if (col.hitboxes.empty()) continue;

			colliderCount++;
			hitboxCount += col.hitboxes.size();
		}

		ImGui::Text("%zu collidable entities, %zu hitboxes", colliderCount, hitboxCount);
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Colliders", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Hitboxes", &m_showHitboxes);
			if (m_showHitboxes) {
				ImGui::Indent();
				ColorEdit("Fill", m_hitboxFill);
				ColorEdit("Outline", m_hitboxOutline);
				ImGui::Unindent();
			}

			ImGui::Checkbox("Compound bounds", &m_showEntityBounds);
			ImGui::SetItemTooltip("the single AABB the entity is inserted into the spatial grid with");
			if (m_showEntityBounds) {
				ImGui::Indent();
				ColorEdit("Bounds", m_boundsOutline);
				ImGui::Unindent();
			}
		}

		if (ImGui::CollapsingHeader("Spatial grid")) {
			ImGui::Checkbox("Cells", &m_showGrid);
			ImGui::Checkbox("Occupancy counts", &m_showCellOccupancy);
			ImGui::Checkbox("Entity -> cell links", &m_showEntityCellLinks);
		}
	}

	void CollisionOverlay::OnDrawOverlay(const DebugContext& ctx, DebugDraw& draw) {
		DrawColliders(ctx, draw);
		DrawSpatialGrid(ctx, draw);
	}

	void CollisionOverlay::DrawColliders(const DebugContext& ctx, DebugDraw& draw) {
		if (!m_showHitboxes && !m_showEntityBounds) return;

		auto view = ctx.registry->GetRegistry().view<const ecs::TransformComponent, const ecs::BoxColliderComponent>();

		for (auto [entity, trans, col] : view.each()) {
			// matches physics_system's IsCollidable: an empty collider has no bounds and
			// is never put in the grid, so drawing one would misrepresent the physics
			if (col.hitboxes.empty()) continue;

			if (m_showHitboxes) {
				for (const ecs::Hitbox& hitbox : col.hitboxes) {
					const sf::FloatRect rect{ trans.pos + hitbox.offset, hitbox.size };

					draw.FilledRect(rect, m_hitboxFill);

					// outline as well as fill, so overlapping hitboxes stay countable
					draw.Rect(rect, m_hitboxOutline);
				}
			}

			if (m_showEntityBounds) {
				draw.Rect(ecs::physics_system::GetEntityBounds(trans, col), m_boundsOutline);
			}
		}
	}

	void CollisionOverlay::DrawSpatialGrid(const DebugContext& ctx, DebugDraw& draw) {
		if (!m_showGrid && !m_showCellOccupancy && !m_showEntityCellLinks) return;

		const auto& grid = ctx.level->GetSpatialGrid();
		const math::GridSize gridSize = grid.GetSize();
		const float cellSize = static_cast<float>(grid.GetCellSize());

		// the world rect the camera can actually see
		const sf::View& view = ctx.camera->GetView();
		const sf::Vector2f viewSize = view.getSize();
		const sf::Vector2f viewTopLeft = view.getCenter() - viewSize / 2.f;

		// clamp in float space before casting, for two separate reasons. casting a
		// negative float to uint32_t is undefined behaviour, and the view sits left of
		// the origin whenever the level is smaller than the window or the camera has no
		// bounds. and an unclamped upper bound walks GetCellEntities off the end of the
		// grid, where Grid::GetCell throws out_of_range
		const auto toRow = [rows = static_cast<float>(gridSize.rows)](float v) {
			return static_cast<uint32_t>(std::clamp(v, 0.f, rows));
		};
		const auto toCol = [cols = static_cast<float>(gridSize.cols)](float v) {
			return static_cast<uint32_t>(std::clamp(v, 0.f, cols));
		};

		const uint32_t startRow = toRow(std::floor(viewTopLeft.y / cellSize));
		const uint32_t endRow = toRow(std::ceil((viewTopLeft.y + viewSize.y) / cellSize));
		const uint32_t startCol = toCol(std::floor(viewTopLeft.x / cellSize));
		const uint32_t endCol = toCol(std::ceil((viewTopLeft.x + viewSize.x) / cellSize));

		// clipped to the visible range. drawing the whole level's grid pushed every off
		// screen line into the vertex batch every frame, so the cost scaled with level
		// size instead of with what you can actually see
		if (m_showGrid) {
			const float left = startCol * cellSize;
			const float right = endCol * cellSize;
			const float top = startRow * cellSize;
			const float bottom = endRow * cellSize;

			for (uint32_t row = startRow; row <= endRow; ++row) {
				const float y = row * cellSize;
				draw.Line({ left, y }, { right, y }, m_gridLine);
			}

			for (uint32_t col = startCol; col <= endCol; ++col) {
				const float x = col * cellSize;
				draw.Line({ x, top }, { x, bottom }, m_gridLine);
			}
		}

		if (!m_showCellOccupancy && !m_showEntityCellLinks) return;

		auto& reg = ctx.registry->GetRegistry();
		const float cellHalf = cellSize / 2.f;

		// one pass for both overlays: they used to walk the same cells twice, asking the
		// grid for the same vector each time
		for (uint32_t row = startRow; row < endRow; ++row) {
			for (uint32_t col = startCol; col < endCol; ++col) {
				// by reference. binding this to `auto` deep copied the cell's entity
				// vector once per visible cell per frame
				const std::vector<entt::entity>& entities = grid.GetCellEntities(row, col);

				// an empty cell costs a string format and a text draw to say nothing,
				// and "0 entities" tiled across the screen buries the cells that matter
				if (entities.empty()) continue;

				const sf::Vector2f cellOrigin{ col * cellSize, row * cellSize };

				if (m_showCellOccupancy) {
					draw.Text(
						{ cellOrigin.x + cellHalf, cellOrigin.y + cellSize / 4.f },
						m_occupiedCellFill,
						std::format("{} entities", entities.size())
					);
				}

				if (!m_showEntityCellLinks) continue;

				const sf::Vector2f cellCentre{ cellOrigin.x + cellHalf, cellOrigin.y + cellHalf };

				for (entt::entity entity : entities) {
					// nothing calls SpatialGrid::EraseEntity anywhere yet, so a destroyed
					// entity leaves its handle behind in the cell. get<> on a dead handle
					// is undefined behaviour, so check before touching it
					if (!reg.valid(entity)) continue;

					const auto* trans = reg.try_get<ecs::TransformComponent>(entity);
					const auto* collider = reg.try_get<ecs::BoxColliderComponent>(entity);
					if (!trans || !collider || collider->hitboxes.empty()) continue;

					const sf::FloatRect bounds = ecs::physics_system::GetEntityBounds(*trans, *collider);
					draw.Arrow(bounds.position + bounds.size / 2.f, cellCentre, m_cellLink);
				}
			}
		}
	}
}

#endif
