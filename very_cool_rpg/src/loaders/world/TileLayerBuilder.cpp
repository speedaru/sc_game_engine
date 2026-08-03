#include <pch.h>
#include <loaders/world/TileLayerBuilder.h>

#include <engine/ecs/Entity.h>
#include <engine/ecs/Components.h>
#include <engine/world/TileLayer.h>
#include <engine/world/YSortedTileLayer.h>
#include <engine/utils/logging.h>

#include <loaders/world/LayerTags.h>
#include <loaders/world/CustomData.h>

namespace ecs = sc::ecs;
namespace world = sc::world;
namespace gfx = sc::graphics;

namespace game::world_loader {
	namespace {
		void SpawnTileCollider(LoadContext& ctx, const std::string& customData, sf::Vector2f tilePos) {
			auto entity = ctx.registry.CreateEntity(std::format("Tile_Collider_{}_{}", (int)tilePos.x, (int)tilePos.y));
			entity.AddComponent<ecs::TransformComponent>(tilePos);

			ApplyCustomData(entity, customData);
		}

		// allows us to build either a TileLayer or a YSortedTileLayer with the same tile-iteration logic
		template <typename LayerClass>
		std::unique_ptr<LayerClass> BuildTileLayer(const ldtk::Layer& ldtkLayer, LoadContext& ctx) {
			world::LayerId layerId{ ldtkLayer.getName(), ldtkLayer.getDefUid() };

			const auto& ldtkTileSet = ldtkLayer.getTileset();
			fs::path tileSetPath = ctx.projectDir / ldtkTileSet.path;

			if (!fs::is_regular_file(tileSetPath)) {
				LOG_W("tile set path was null for layer: %s", ldtkLayer.getName().c_str());
				return nullptr;
			}

			std::shared_ptr<gfx::TileSet> tileSet = ctx.tileSetManager.GetOrLoad(tileSetPath, ldtkTileSet.tile_size);

			auto layer = std::make_unique<LayerClass>(layerId, tileSet);

			std::vector<world::TileInstance> tileInstances;
			tileInstances.reserve(ldtkLayer.allTiles().size());

			for (const auto& tile : ldtkLayer.allTiles()) {
				world::TileInstance instance;
				instance.pixelPos = sf::Vector2f(static_cast<float>(tile.getPosition().x), static_cast<float>(tile.getPosition().y));

				const std::string& tileData = ldtkTileSet.getTileCustomData(tile.tileId);
				if (!tileData.empty()) {
					SpawnTileCollider(ctx, tileData, instance.pixelPos);
				}

				auto rect = tile.getTextureRect();
				instance.textureRect = sf::IntRect({ rect.x, rect.y }, { rect.width, rect.height });

				instance.flipX = tile.flipX;
				instance.flipY = tile.flipY;

				tileInstances.push_back(instance);
			}

			layer->LoadTiles(tileInstances);
			return layer;
		}
	}

	bool TileLayerBuilder::CanHandle(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>&) const {
		return ldtkLayer.hasTileset() && ldtkLayer.getType() != ldtk::LayerType::Entities;
	}

	std::unique_ptr<world::ILevelLayer> TileLayerBuilder::Build(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>&, LoadContext& ctx) const {
		return BuildTileLayer<world::TileLayer>(ldtkLayer, ctx);
	}

	bool YSortedTileLayerBuilder::CanHandle(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>& tags) const {
		return ldtkLayer.hasTileset() && ldtkLayer.getType() != ldtk::LayerType::Entities && tags.contains(TAG);
	}

	std::unique_ptr<world::ILevelLayer> YSortedTileLayerBuilder::Build(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>&, LoadContext& ctx) const {
		return BuildTileLayer<world::YSortedTileLayer>(ldtkLayer, ctx);
	}
}
