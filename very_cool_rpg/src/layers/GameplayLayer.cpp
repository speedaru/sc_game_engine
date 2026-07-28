#include <pch.h>
#include <layers/GameplayLayer.h>

#include <LDtkLoader/Project.hpp>

#include <engine/ecs/Components.h>
#include <engine/graphics/Texture2D.h>
#include <engine/graphics/render2d.h>

#include <constants.h>
#include <components/PlayerComponents.h>
#include <blueprints/BlueprintRegistry.h>
#include <utils/render_utils.h>
namespace ecs = sc::ecs;
namespace ldr = sc::loader;
namespace gfx = sc::graphics;

namespace game {
	static sc::loader::LayerData ExtractLayerData(const ldtk::Layer& layer) {
		ldr::LayerData out;
		out.gridWidth = layer.getGridSize().x;
		out.gridHeight = layer.getGridSize().y;
		out.tileSize = layer.getCellSize();

		// fill with null tiles
		out.tiles.assign(out.gridWidth * out.gridHeight, 0);

		// map to our array
		for (const auto& tile : layer.allTiles()) {
			int gridX = tile.getGridPosition().x;
			int gridY = tile.getGridPosition().y;
			int index = (gridY * out.gridWidth) + gridX;

			out.tiles[index] = tile.tileId;
		}

		return out;
	}

    void GameplayLayer::OnAttach() {
		game::blueprints::RegisterAll(m_entityFactory);


		// loader
		ldtk::Project ldtkLoader;
		fs::path projectDir = fs::absolute(ASSETS_DIR);
		ldtkLoader.loadFromFile((projectDir / "world_test1.ldtk").string());

		for (const auto& world : ldtkLoader.allWorlds()) {
			LOG_D("world name: '%s' (len: %llu)", world.getName().c_str(), world.getName().length());
		}

		// load level
		const auto& world = ldtkLoader.getWorld("");
		const auto& level = world.getLevel("World_Level_0");
		const auto& backgroundLayer = level.getLayer("Background");

		// load layers
		LOG_D("level size: %d %d", level.size.x, level.size.y);
		m_level = std::make_shared<gfx::Level>(sf::Vector2i(level.size.x, level.size.y));
		for (const auto& layer : level.allLayers()) {
			LOG_D("current layer: %s", layer.getName().c_str());
			if (!layer.hasTileset()) {
				continue;
			}

			auto& ldtkTileSet = layer.getTileset();
			auto tileSet = std::make_shared<gfx::TileSet>(projectDir / ldtkTileSet.path, ldtkTileSet.tile_size);
			auto tileMap = std::make_shared<gfx::TileMap>(tileSet);

			auto layerData = ExtractLayerData(layer);
			tileMap->Load(layerData.tiles, layerData.gridWidth, layerData.gridHeight);
			m_level->AddLayer(tileMap);
		}

		// load entities
		for (const auto& ldtkEntity : level.getLayer("Entities").allEntities()) {
			LOG_D("entity name: %s", ldtkEntity.getName().c_str());
			m_entityFactory.Spawn(m_scene, ldtkEntity, projectDir);
		}

		// find player
		auto view = m_scene.GetRegistry().view<components::PlayerTag>();
		for (auto entityHandle : view) {
			m_player = sc::ecs::Entity(entityHandle, &m_scene);
			break; // only 1 player
		}
    }

    void GameplayLayer::OnFixedUpdate(float timeStep) {
		if (m_player && m_player.HasComponent<ecs::TransformComponent>()) {
			const auto& trans = m_player.GetComponent<ecs::TransformComponent>();
			//trans.pos.x += 1.f;
			//trans.pos.y += 1.f;
			sf::Vector2f cameraPos = trans.pos;

			// limit camera in map edges
			cameraPos.x = std::max(cameraPos.x, m_camera.GetHalfSize().x);
			cameraPos.x = std::min(cameraPos.x, m_level->GetSize().x - m_camera.GetHalfSize().x);
			cameraPos.y = std::max(cameraPos.y, m_camera.GetHalfSize().y);
			cameraPos.y = std::min(cameraPos.y, m_level->GetSize().y - m_camera.GetHalfSize().y);

			m_camera.SetPosition(cameraPos);
		}
    }

    void GameplayLayer::OnRender(sf::RenderWindow& window) {
        auto view = m_scene.GetRegistry().view<const ecs::TransformComponent, ecs::SpriteComponent>();

        sc::graphics::render2d::BeginScene();

		// render world
		if (m_level) {
			gfx::render2d::SubmitLevel(*m_level);
		}

		// render entities
        for (auto [entityHandle, trans, spr] : view.each()) {
            sc::ecs::Entity entity(entityHandle, &m_scene);
            render_utils::SubmitEntity(entity);
        }

        sc::graphics::render2d::EndScene(window, m_camera);
    }
}
