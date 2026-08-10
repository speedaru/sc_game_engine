#pragma once
#include <engine/debug/debug_config.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <SFML/Graphics/Color.hpp>

#include <engine/debug/IDebugModule.h>

namespace sc::debug::modules {
	// draws what the physics system actually sees, from both of its broad phases: entity
	// hitboxes and their compound AABBs out of the spatial grid, and static level geometry
	// out of the collision layer
	class CollisionOverlay : public IDebugModule {
	public:
		const char* Name() const override { return "Collisions"; }
		const char* Category() const override { return "Physics"; }

		void OnDrawUI(const DebugContext& ctx) override;
		void OnDrawOverlay(const DebugContext& ctx, DebugDraw& draw) override;

	private:
		void DrawColliders(const DebugContext& ctx, DebugDraw& draw);
		void DrawSpatialGrid(const DebugContext& ctx, DebugDraw& draw);
		void DrawTileCollision(const DebugContext& ctx, DebugDraw& draw);

	private:
		// entity colliders
		bool m_showHitboxes = true;
		bool m_showEntityBounds = false;
		sf::Color m_hitboxFill{ 255, 0, 0, 100 };
		sf::Color m_hitboxOutline{ 255, 80, 80, 200 };
		sf::Color m_boundsOutline{ 255, 255, 0, 180 };

		// static tile collision
		bool m_showTileShapes = true;
		bool m_showTileCells = false;
		bool m_showShapeIndices = false;
		sf::Color m_tileFill{ 0, 140, 255, 90 };
		sf::Color m_tileOutline{ 120, 200, 255, 200 };
		sf::Color m_tileCellLine{ 255, 255, 255, 40 };
		sf::Color m_shapeIndexText{ 200, 230, 255, 230 };

		// spatial grid
		bool m_showGrid = false;
		bool m_showCellOccupancy = true;
		bool m_showEntityCellLinks = false;
		sf::Color m_gridLine{ 80, 160, 255, 140 };
		sf::Color m_occupiedCellFill{ 255, 255, 255, 230 };
		sf::Color m_cellLink{ 80, 255, 160, 160 };
	};
}

#endif
