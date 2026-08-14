#include <pch.h>
#include <loaders/world/collision_baker.h>

#include <engine/utils/logging.h>

#include <loaders/world/TileColliderCache.h>

namespace math = sc::math;
namespace world = sc::world;

namespace game::world_loader {
	namespace {
		// used if a level has no collision source at all
		constexpr uint32_t FALLBACK_CELL_SIZE = 16u;

		// choose smallest cell size amongst collision layers
		uint32_t ChooseCellSize(const ldtk::Level& ldtkLevel) {
			uint32_t cellSize = std::numeric_limits<uint32_t>::max();

			for (const auto& ldtkLayer : ldtkLevel.allLayers()) {
				// collisions only come from tile and intgrid layers
				if (ldtkLayer.getType() == ldtk::LayerType::Entities
					|| ldtkLayer.getType() == ldtk::LayerType::AutoLayer) {
					continue;
				}

				cellSize = std::min(cellSize, static_cast<uint32_t>(ldtkLayer.getCellSize()));
			}

			if (cellSize == std::numeric_limits<uint32_t>::max()) {
				LOG_W("level '%s' has no collision source, using a %upx collision grid", ldtkLevel.name.c_str(), FALLBACK_CELL_SIZE);
				return FALLBACK_CELL_SIZE;
			}

			return cellSize;
		}
	}

	void BakeTileLayer(const ldtk::Layer& ldtkLayer, TileColliderCache& tileColliders, world::CollisionLayerBuilder& out) {
		const ldtk::Tileset& tileset = ldtkLayer.getTileset();

		for (const auto& tile : ldtkLayer.allTiles()) {
			const std::vector<math::Hitbox>& hitboxes = tileColliders.Get(tileset, tile.tileId);
			if (hitboxes.empty()) continue;

			const ldtk::IntPoint pos = tile.getPosition();
			const sf::Vector2f tileOrigin{ static_cast<float>(pos.x), static_cast<float>(pos.y) };

			for (const math::Hitbox& hitbox : hitboxes) {
				out.AddBox(sf::FloatRect{ tileOrigin + hitbox.offset, hitbox.size });
			}
		}
	}

	void BakeIntGridLayer(const ldtk::Layer& ldtkLayer, TileColliderCache& tileColliders, world::CollisionLayerBuilder& out) {
		if (!ldtkLayer.hasTileset()) {
			LOG_W("skipping collision baking in intgrid layer %u bcs no tileset", ldtkLayer.getDefUid());
			return;
		}

		// use the collisions of each painted tile object
		BakeTileLayer(ldtkLayer, tileColliders, out);
	}

	world::CollisionLayer BakeLevelCollision(const ldtk::Level& ldtkLevel, TileColliderCache& tileColliders) {
		const uint32_t cellSize = ChooseCellSize(ldtkLevel);

		// round up to cover bottom right edge of level
		const math::GridSize size{
			.rows = (static_cast<uint32_t>(ldtkLevel.size.y) + cellSize - 1) / cellSize,
			.cols = (static_cast<uint32_t>(ldtkLevel.size.x) + cellSize - 1) / cellSize
		};

		world::CollisionLayerBuilder builder({ 0.f, 0.f }, cellSize, size);

		for (const auto& ldtkLayer : ldtkLevel.allLayers()) {
			// keep invisible layers
			if (ldtkLayer.getType() == ldtk::LayerType::IntGrid) {
				BakeIntGridLayer(ldtkLayer,  tileColliders, builder);
			}
			else if (ldtkLayer.getType() == ldtk::LayerType::Tiles) {
				BakeTileLayer(ldtkLayer, tileColliders, builder);
			}
		}

		return builder.Build();
	}
}
