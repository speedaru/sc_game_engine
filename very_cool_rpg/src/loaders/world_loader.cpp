#include <pch.h>
#include <loaders/world_loader.h>

#include <engine/graphics/TileSet.h>
#include <engine/ecs/Entity.h>
#include <engine/world/TileLayer.h>
#include <engine/world/YSortedTileLayer.h>
#include <engine/world/EntityLayer.h>
#include <engine/world/CollisionLayer.h>
#include <engine/utils/logging.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace game;
using namespace game::world_loader;
namespace ecs = sc::ecs;
namespace world = sc::world;
namespace gfx = sc::graphics;

// private functions
namespace {
	int GetTileIdFromEntity(const ldtk::Entity& entity, const ldtk::Tileset& tileset) {
		const auto rect = entity.getTextureRect();

		const int stride = tileset.tile_size + tileset.spacing;
		const int columns = tileset.texture_size.x / tileset.tile_size;

		return ((rect.y - tileset.padding) / stride) * columns +
			((rect.x - tileset.padding) / stride);
	}

	inline sf::Vector2f CalcPivotOffset(const ldtk::Entity& ent) {
		auto rect = ent.getTextureRect();
		auto pivot = ent.getPivot();
		return { pivot.x * rect.width, pivot.y * rect.height };
	}

	void AddBoxCollider(const ecs::Entity& entity, const json& j, sf::Vector2f origin = { 0.f, 0.f }) {
		auto& collider = entity.AddComponent<ecs::BoxColliderComponent>();

		// parse hitboxes and add them to the component
		for (const auto& box : j) {
			collider.hitboxes.emplace_back(
				box.value("x", 0.f) - origin.x,
				box.value("y", 0.f) - origin.y,
				box.value("w", 16.f),
				box.value("h", 16.f)
			);
		}
	}

	void ProcessCustomData(ecs::Registry& registry, const std::string& customData, const sf::Vector2f& tilePos) {
		try {
			auto j = json::parse(customData);

			// ensure the data is an array of hitboxes
			if (j.is_array() && !j.empty()) {
				// create physics entity
				auto entity = registry.CreateEntity(std::format("Tile_Collider_{}_{}", (int)tilePos.x, (int)tilePos.y));
				entity.AddComponent<ecs::TransformComponent>(tilePos);

				AddBoxCollider(entity, j);
			}
			else {
				LOG_W("Custom data for tile at %.f %.f is not a valid JSON array.", tilePos.x, tilePos.y);
			}
		}
		catch (const json::parse_error& e) {
			LOG_E("Failed to parse JSON hitbox for tile %.f %.f. Error: %s", tilePos.x, tilePos.y, e.what());
		}
	}

	// allows us to return either a TileLayer or a YSortedTileLayer
	template <typename LayerClass>
	std::unique_ptr<LayerClass> ProcessGenericTileLayer(const ldtk::Layer& ldtkLayer, ecs::Registry& registry, const fs::path& projectDir) {
		world::LayerId layerId{ ldtkLayer.getName(), ldtkLayer.getDefUid() };

		auto& ldtkTileSet = ldtkLayer.getTileset();
		fs::path tileSetPath = projectDir / ldtkTileSet.path;

		if (!fs::is_regular_file(tileSetPath)) {
			LOG_W("tile set path was null for layer: %s", ldtkLayer.getName().c_str());
			return nullptr;
		}

		std::shared_ptr<gfx::TileSet> tileSet = std::make_shared<gfx::TileSet>(tileSetPath, ldtkTileSet.tile_size);

		// instantiate whichever class was passed in the template parameter
		auto layer = std::make_unique<LayerClass>(layerId, tileSet);

		std::vector<world::TileInstance> tileInstances;
		tileInstances.reserve(ldtkLayer.allTiles().size());

		for (const auto& tile : ldtkLayer.allTiles()) {
			world::TileInstance instance;
			instance.pixelPos = sf::Vector2f(static_cast<float>(tile.getPosition().x), static_cast<float>(tile.getPosition().y));

			const std::string& tileData = ldtkTileSet.getTileCustomData(tile.tileId);
			if (!tileData.empty()) {
				ProcessCustomData(registry, tileData, instance.pixelPos);
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

	std::unique_ptr<world::EntityLayer> ProcessEntityLayer(const ldtk::Layer& ldtkLayer, ecs::Registry& registry, factories::EntityFactory& entityFactory) {
		world::LayerId layerId{ ldtkLayer.getName(), ldtkLayer.getDefUid() };
		auto entityLayer = std::make_unique<world::EntityLayer>(layerId);

		// process collisions

		for (const auto& ldtkEntity : ldtkLayer.allEntities()) {
			// get entity texture tile id
			const auto& tileset = *ldtkEntity.getEntityDef()->tileset;
			int tileId = GetTileIdFromEntity(ldtkEntity, tileset);

			const std::string& customData = tileset.getTileCustomData(tileId);
			ecs::Entity entity = entityFactory.Spawn(registry, ldtkEntity);

			// parse custom data
			if (!customData.empty()) {
				sf::Vector2f tilePos((float)ldtkEntity.getPosition().x, (float)ldtkEntity.getPosition().y);
				AddBoxCollider(entity, json::parse(customData), CalcPivotOffset(ldtkEntity));

				const auto& box = entity.GetComponent<ecs::BoxColliderComponent>();
				LOG_D("added box colider for entity: %s (%llu hitboxes)", ldtkEntity.getName().c_str(), box.hitboxes.size());
			}
		}

		return entityLayer;
	}

	std::shared_ptr<world::Level> LoadLevel(const ldtk::Level& ldtkLevel, const fs::path& projectDir, ecs::Registry& registry, factories::EntityFactory& entityFactory) {
		world::LevelId levelId{ ldtkLevel.name, ldtkLevel.uid };
		auto engineLevel = std::make_shared<world::Level>(levelId, sf::Vector2i(ldtkLevel.size.x, ldtkLevel.size.y));

		// Iterate layers from BOTTOM to TOP (reverse iterator)
		for (auto it = ldtkLevel.allLayers().rbegin(); it != ldtkLevel.allLayers().rend(); ++it) {
			const auto& ldtkLayer = *it;

			// skip invisible layers
			if (!ldtkLayer.isVisible()) continue;

			using ldtk::LayerType;
			const LayerType layerType = ldtkLayer.getType();
			const std::string layerName = ldtkLayer.getName();

			const bool hasTileSet = ldtkLayer.hasTileset();
			const bool entityLayer = layerType == ldtk::LayerType::Entities;

			// layers with visual tiles
			if (hasTileSet && !entityLayer) {
				// special y sorted tile layer
				if (layerName.find(YSORTED_LAYER_TAG) != std::string::npos) {
					engineLevel->AddLayer(ProcessGenericTileLayer<world::YSortedTileLayer>(ldtkLayer, registry, projectDir));
				}
				// standard tile layer
				else {
					engineLevel->AddLayer(ProcessGenericTileLayer<world::TileLayer>(ldtkLayer, registry, projectDir));
				}
			}
			// entity layers
			else if (ldtkLayer.getType() == ldtk::LayerType::Entities) {
				engineLevel->AddLayer(ProcessEntityLayer(ldtkLayer, registry, entityFactory));
			}
		}

		return engineLevel;
	}
}

namespace game::world_loader {
	std::shared_ptr<world::World> Load(const fs::path& projectFilePath, ecs::Registry& registry, factories::EntityFactory& entityFactory) {
        auto world = std::make_shared<world::World>();
        ldtk::Project ldtkProject;
        if (!fs::exists(projectFilePath)) {
            LOG_E("project file: '%s' doesn't exist", projectFilePath.string().c_str());
            return nullptr;
        }

		ldtkProject.loadFromFile(projectFilePath.string());

        // load all levels
        const fs::path projectDir = projectFilePath.parent_path();
        for (const auto& ldtkLevel : ldtkProject.getWorld("").allLevels()) {
            world->AddLevel(LoadLevel(ldtkLevel, projectDir, registry, entityFactory));
        }

        return world;
	}
}