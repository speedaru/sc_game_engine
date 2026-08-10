#include <pch.h>
#include <loaders/world/collision_baker.h>

#include <engine/utils/logging.h>

#include <loaders/world/TileColliderCache.h>

namespace math = sc::math;
namespace world = sc::world;

namespace game::world_loader {
	namespace {
		// used only if a level somehow has no collision source at all, so the grid is
		// well formed rather than degenerate
		constexpr uint32_t FALLBACK_CELL_SIZE = 16u;

		bool IsIntGridSource(const ldtk::Layer& ldtkLayer) {
			return ldtkLayer.getType() == ldtk::LayerType::IntGrid;
		}

		bool IsTileSource(const ldtk::Layer& ldtkLayer) {
			// an IntGrid layer with auto rules also has a tileset, but its collision comes
			// from the painted values, not from the tiles those values happen to render
			return ldtkLayer.getType() != ldtk::LayerType::Entities
				&& ldtkLayer.getType() != ldtk::LayerType::IntGrid
				&& ldtkLayer.hasTileset();
		}

		// the coarsest contributing source.
		//
		// going finer would split that source's tiles across several cells for no benefit,
		// and every fragment is an extra candidate plus an interior edge for the sweep to
		// catch on. going coarser is worse: a cell then holds many tiles' worth of geometry
		// and every distinct arrangement becomes its own palette entry
		uint32_t ChooseCellSize(const ldtk::Level& ldtkLevel) {
			uint32_t cellSize = 0u;

			for (const auto& ldtkLayer : ldtkLevel.allLayers()) {
				if (!IsIntGridSource(ldtkLayer) && !IsTileSource(ldtkLayer)) continue;

				cellSize = std::max(cellSize, static_cast<uint32_t>(ldtkLayer.getCellSize()));
			}

			if (cellSize == 0u) {
				LOG_W("level '%s' has no collision source, using a %upx collision grid", ldtkLevel.name.c_str(), FALLBACK_CELL_SIZE);
				return FALLBACK_CELL_SIZE;
			}

			return cellSize;
		}
	}

	void BakeTileLayer(const ldtk::Layer& ldtkLayer, TileColliderCache& tileColliders, world::CollisionLayerBuilder& out) {
		const ldtk::Tileset& tileset = ldtkLayer.getTileset();

		for (const auto& tile : ldtkLayer.allTiles()) {
			// cached per (tileset, tileId): the same tile id repeats hundreds of times in a
			// level, and re-parsing its custom data json each time was most of the load
			const std::vector<math::Hitbox>& hitboxes = tileColliders.Get(tileset, tile.tileId);
			if (hitboxes.empty()) continue;

			const ldtk::IntPoint pos = tile.getPosition();
			const sf::Vector2f tileOrigin{ static_cast<float>(pos.x), static_cast<float>(pos.y) };

			for (const math::Hitbox& hitbox : hitboxes) {
				out.AddBox(sf::FloatRect{ tileOrigin + hitbox.offset, hitbox.size });
			}
		}
	}

	void BakeIntGridLayer(const ldtk::Layer& ldtkLayer, world::CollisionLayerBuilder& out) {
		const ldtk::IntPoint gridSize = ldtkLayer.getGridSize();
		const float cellSize = static_cast<float>(ldtkLayer.getCellSize());

		for (int y = 0; y < gridSize.y; y++) {
			for (int x = 0; x < gridSize.x; x++) {
				// every non zero value is solid. a second value (one way, water, ice) is
				// what turns this into a value -> TileShape table, which is game side config
				if (ldtkLayer.getIntGridVal(x, y).value == 0) continue;

				out.AddBox(sf::FloatRect{
					{ x * cellSize, y * cellSize },
					{ cellSize, cellSize }
				});
			}
		}
	}

	world::CollisionLayer BakeLevelCollision(const ldtk::Level& ldtkLevel, TileColliderCache& tileColliders) {
		const uint32_t cellSize = ChooseCellSize(ldtkLevel);

		// round up: truncating division leaves the level's right/bottom edge uncovered
		const math::GridSize size{
			.rows = (static_cast<uint32_t>(ldtkLevel.size.y) + cellSize - 1) / cellSize,
			.cols = (static_cast<uint32_t>(ldtkLevel.size.x) + cellSize - 1) / cellSize
		};

		world::CollisionLayerBuilder builder({ 0.f, 0.f }, cellSize, size);

		for (const auto& ldtkLayer : ldtkLevel.allLayers()) {
			// deliberately NOT gated on isVisible(), unlike the render layer loop. a
			// collision IntGrid is normally hidden while editing, and hiding a layer in the
			// editor must not silently delete the level's collision
			if (IsIntGridSource(ldtkLayer)) {
				BakeIntGridLayer(ldtkLayer, builder);
			}
			else if (IsTileSource(ldtkLayer)) {
				BakeTileLayer(ldtkLayer, tileColliders, builder);
			}
		}

		return builder.Build();
	}
}
