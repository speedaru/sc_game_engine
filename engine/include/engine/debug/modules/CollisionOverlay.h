#pragma once
#include <engine/debug/debug_config.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <SFML/Graphics/Color.hpp>

#include <engine/debug/IDebugModule.h>

namespace sc::debug::modules {
	// draws what the physics system actually sees: every hitbox, the single compound
	// AABB each entity is inserted into the spatial grid with, and - once SpatialGrid
	// exposes a read API - the grid itself
	class CollisionOverlay : public IDebugModule {
	public:
		const char* Name() const override { return "Collisions"; }
		const char* Category() const override { return "Physics"; }

		void OnDrawUI(const DebugContext& ctx) override;
		void OnDrawOverlay(const DebugContext& ctx, DebugDraw& draw) override;

	private:
		void DrawColliders(const DebugContext& ctx, DebugDraw& draw);
		void DrawSpatialGrid(const DebugContext& ctx, DebugDraw& draw);

	private:
		// colliders
		bool m_showHitboxes = true;
		bool m_showEntityBounds = false;
		sf::Color m_hitboxFill{ 255, 0, 0, 100 };
		sf::Color m_hitboxOutline{ 255, 80, 80, 200 };
		sf::Color m_boundsOutline{ 255, 255, 0, 180 };

		// spatial grid - not wired up yet, see DrawSpatialGrid for what it's waiting on
		bool m_showGrid = false;
		bool m_showCellOccupancy = true;
		bool m_showEntityCellLinks = false;
		sf::Color m_gridLine{ 80, 160, 255, 140 };
		sf::Color m_occupiedCellFill{ 255, 255, 255, 230 };
		sf::Color m_cellLink{ 80, 255, 160, 160 };
	};
}

#endif
