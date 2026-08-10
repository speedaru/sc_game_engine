#include <pch.h>
#include <layers/GameplayLayer.h>

#include <LDtkLoader/Project.hpp>

#include <engine/input/Input.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/systems/render_system.h>
#include <engine/graphics/Texture2D.h>
#include <engine/renderer/render2d.h>
#include <engine/debug/debug_system.h>

#include <constants.h>
#include <components/GameComponents.h>
#include <enums/InputActions.h>
#include <blueprints/BlueprintRegistry.h>
#include <systems/player_input.h>
#include <systems/character_movement.h>

using Key = sf::Keyboard::Key;
namespace core = sc::core;
namespace dbg = sc::debug;
namespace input = sc::input;
namespace ecs = sc::ecs;
namespace gfx = sc::graphics;

namespace game {
    void GameplayLayer::OnAttach() {
		// load keybinds
		auto gameplayInput = std::make_shared<input::InputContext>();
		gameplayInput->Bind(Key::W, static_cast<int32_t>(InputAction::MoveUp));
		gameplayInput->Bind(Key::A, static_cast<int32_t>(InputAction::MoveLeft));
		gameplayInput->Bind(Key::S, static_cast<int32_t>(InputAction::MoveDown));
		gameplayInput->Bind(Key::D, static_cast<int32_t>(InputAction::MoveRight));
		gameplayInput->Bind(Key::F2, static_cast<int32_t>(InputAction::SpawnEnt));
		input::PushContext(gameplayInput);

		fs::path projectDir = fs::absolute(ASSETS_DIR);
		fs::path project = projectDir / "world_test1.ldtk";

		// load entity blueprints
		blueprints::RegisterAll(m_entityFactory);

		// load world
		m_world = world_loader::Load(project, m_registry, m_entityFactory, m_tileSetManager);
		if (!m_world) {
			return;
		}

		// get level
		m_currentLevel = m_world->GetLevel("World_Level_0").get();
		if (!m_currentLevel) {
			LOG_W("failed to get level 0");
			return;
		}

		// the factory spawned into whichever level the loader was building at the time;
		// from here on runtime spawns belong to the level actually being played
		m_entityFactory.SetCurrentLevel(m_currentLevel);

		// remember which layer runtime spawns should render in
		for (const auto& layer : m_currentLevel->GetLayers()) {
			if (layer->GetType() == sc::world::LayerType::Entity) {
				m_entityLayerUid = layer->GetId().uid;
				break;
			}
		}

		if (m_entityLayerUid < 0) {
			LOG_W("level has no entity layer, so anything spawned at runtime won't render");
		}

		// apply bounds to camera
		m_camera.SetBounds({ 0.f, 0.f }, static_cast<sf::Vector2f>(m_currentLevel->GetSize()));

		// get player reference
		const auto view = m_registry.GetRegistry().view<const components::PlayerTag>();
		for (auto entityHandle : view) {
			m_player = sc::ecs::Entity(entityHandle, &m_registry);
			break; // only 1 player
		}

		if (!m_player) {
			LOG_W("no player found !");
		}
    }

    void GameplayLayer::OnFixedUpdate(float timeStep) {
		// handle player keybinds
		systems::UpdatePlayerInput(m_registry);

		// calculate every entity movement
		systems::UpdateCharacterMovement(m_registry, timeStep);

		// move entities in engine
		ecs::physics_system::UpdateKinematics(m_registry, *m_currentLevel, timeStep);

		// center camera on player
		if (m_player && m_player.HasComponent<ecs::TransformComponent>()) {
			const auto& trans = m_player.GetComponent<ecs::TransformComponent>();
			sf::Vector2f cameraPos = trans.pos;

			m_camera.SetPosition(cameraPos);
		}
    }

    void GameplayLayer::OnUpdate(float deltaTime) {
		// published once per frame rather than per physics tick: OnFixedUpdate runs zero
		// or more times off the accumulator, and deltaTime only exists here.
		// runs before DebugLayer::OnUpdate because layers update bottom to top
		SC_DEBUG_ONLY(dbg::SetContext(dbg::DebugContext{
			.registry = &m_registry,
			.level = m_currentLevel,
			.camera = &m_camera,
			.deltaTime = deltaTime
		}));

		if (input::IsActionActive(static_cast<uint32_t>(InputAction::SpawnEnt))) {
			const auto& trans = m_player.GetComponent<ecs::TransformComponent>();
			m_entityFactory.Spawn(entities::EntityType::Dragon, entities::SpawnParams{
				.position = { trans.pos + sf::Vector2f{ 50.f, 50.f } },
				.layerUid = m_entityLayerUid
			});
		}
    }

    void GameplayLayer::OnRender(sf::RenderWindow& window) {
		ecs::render_system::RenderWorld(m_registry, *m_currentLevel, window, m_camera);

		// collision boxes used to be drawn by hand here. that now lives in the
		// CollisionOverlay debug module, which batches them instead of issuing a draw
		// call per hitbox, and can be toggled at runtime
    }
}
