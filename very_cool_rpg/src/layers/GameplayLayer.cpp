#include <pch.h>
#include <layers/GameplayLayer.h>

#include <LDtkLoader/Project.hpp>

#include <engine/ecs/Components.h>
#include <engine/ecs/systems/PhysicsSystem.h>
#include <engine/graphics/Texture2D.h>
#include <engine/graphics/render2d.h>

#include <constants.h>
#include <components/GameComponents.h>
#include <enums/InputActions.h>
#include <blueprints/BlueprintRegistry.h>
#include <systems/player_input.h>
#include <systems/character_movement.h>
#include <utils/render_utils.h>

using Key = sf::Keyboard::Key;
namespace core = sc::core;
namespace ecs = sc::ecs;
namespace gfx = sc::graphics;

namespace game {
	static gfx::MapData ExtractMapData(const ldtk::Layer& layer) {
		gfx::MapData out;
		out.width = layer.getGridSize().x;
		out.height = layer.getGridSize().y;

		// fill with null tiles
		out.tiles.assign(out.width * out.height, 0);

		// map to our array
		for (const auto& tile : layer.allTiles()) {
			int gridX = tile.getGridPosition().x;
			int gridY = tile.getGridPosition().y;
			int index = (gridY * out.width) + gridX;

			out.tiles[index] = tile.tileId;
		}

		return out;
	}

    void GameplayLayer::OnAttach() {
		// load keybinds
		auto gameplayInput = std::make_shared<core::InputContext>();
		gameplayInput->Bind(Key::W, static_cast<int32_t>(InputAction::MoveUp));
		gameplayInput->Bind(Key::A, static_cast<int32_t>(InputAction::MoveLeft));
		gameplayInput->Bind(Key::S, static_cast<int32_t>(InputAction::MoveDown));
		gameplayInput->Bind(Key::D, static_cast<int32_t>(InputAction::MoveRight));
		core::input::PushContext(gameplayInput);

		// loader
		ldtk::Project ldtkLoader;
		fs::path projectDir = fs::absolute(ASSETS_DIR);
		ldtkLoader.loadFromFile((projectDir / "world_test1.ldtk").string());

		// load level and layers
		const auto& world = ldtkLoader.getWorld("");
		const auto& level = world.getLevel("World_Level_0");

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

			auto mapData = ExtractMapData(layer);
			tileMap->Load(mapData);
			m_level->AddLayer(tileMap);
		}

		// load entities into ecs
		blueprints::RegisterAll(m_entityFactory, projectDir);
		for (const auto& ldtkEntity : level.getLayer("Entities").allEntities()) {
			LOG_D("entity name: %s", ldtkEntity.getName().c_str());
			m_entityFactory.Spawn(m_scene, ldtkEntity);
		}

		// get player reference
		auto view = m_scene.GetRegistry().view<components::PlayerTag>();
		for (auto entityHandle : view) {
			m_player = sc::ecs::Entity(entityHandle, &m_scene);
			break; // only 1 player
		}
    }

    void GameplayLayer::OnFixedUpdate(float timeStep) {
		// handle player keybinds
		systems::UpdatePlayerInput(m_scene);

		// calculate every entity movement
		systems::UpdateCharacterMovement(m_scene, timeStep);

		// move entities in engine
		ecs::systems::UpdateKinematics(m_scene, timeStep);

		// center camera on player
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
