#include <pch.h>
#include <loaders/world_loader.h>

#include <engine/graphics/TileSet.h>
#include <engine/ecs/Entity.h>
#include <engine/world/TileLayer.h>
#include <engine/world/YSortedTileLayer.h>
#include <engine/world/EntityLayer.h>
#include <engine/world/CollisionLayer.h>
#include <engine/utils/logging.h>

#include <LDtkLoader/Project.hpp>

namespace ecs = sc::ecs;
namespace world = sc::world;
namespace gfx = sc::graphics;

namespace game::world_loader {
	namespace {
        // allows us to return either a TileLayer or a YSortedTileLayer
		template <typename LayerClass>
		std::unique_ptr<LayerClass> ProcessGenericTileLayer(const ldtk::Layer& ldtkLayer, const fs::path& projectDir) {
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

            for (const auto& ldtkEntity : ldtkLayer.allEntities()) {
                entityFactory.Spawn(registry, ldtkEntity);
            }

            return entityLayer;
        }

        std::unique_ptr<world::CollisionLayer> ProcessCollisionLayer(const ldtk::Layer& ldtkLayer) {
            world::LayerId layerId{ ldtkLayer.getName(), ldtkLayer.getDefUid() };

            int gridWidth = ldtkLayer.getGridSize().x;
            int gridHeight = ldtkLayer.getGridSize().y;
            int gridSize = ldtkLayer.getCellSize();

            auto collisionLayer = std::make_unique<world::CollisionLayer>(layerId, gridWidth, gridHeight, gridSize);

            std::vector<int32_t> gridData(gridWidth * gridHeight, 0);
            for (const auto& tile : ldtkLayer.allTiles()) {
                int index = (tile.getGridPosition().y * gridWidth) + tile.getGridPosition().x;
                gridData[index] = tile.tileId;
            }

            collisionLayer->SetData(gridData);
            return collisionLayer;
        }

        std::shared_ptr<world::Level> LoadLevel(const ldtk::Level& ldtkLevel, const fs::path& projectDir, ecs::Registry& registry, factories::EntityFactory& entityFactory) {
            world::LevelId levelId{ ldtkLevel.name, ldtkLevel.uid };
            auto engineLevel = std::make_shared<world::Level>(levelId, sf::Vector2i(ldtkLevel.size.x, ldtkLevel.size.y));

            // Iterate layers from BOTTOM to TOP (reverse iterator)
            for (auto it = ldtkLevel.allLayers().rbegin(); it != ldtkLevel.allLayers().rend(); ++it) {
                const auto& ldtkLayer = *it;

                using ldtk::LayerType;
                const LayerType layerType = ldtkLayer.getType();
                const std::string layerName = ldtkLayer.getName();

                const bool hasTileSet = ldtkLayer.hasTileset();
                const bool entityLayer = layerType == ldtk::LayerType::Entities;

                // layers with visual tiles
                if (hasTileSet && !entityLayer) {
                    // special y sorted tile layer
                    if (layerName.find(YSORTED_LAYER_TAG) != std::string::npos) {
                        engineLevel->AddLayer(ProcessGenericTileLayer<world::YSortedTileLayer>(ldtkLayer, projectDir));
                    }
                    // standard tile layer
                    else {
                        engineLevel->AddLayer(ProcessGenericTileLayer<world::TileLayer>(ldtkLayer, projectDir));
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