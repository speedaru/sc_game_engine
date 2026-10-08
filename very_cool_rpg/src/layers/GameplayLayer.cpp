#include <pch.h>
#include <layers/GameplayLayer.h>

#include <LDtkLoader/Project.hpp>

#include <engine/input/Input.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/systems/render_system.h>
#include <engine/ecs/systems/lifecycle_system.h>
#include <engine/graphics/Texture2D.h>
#include <engine/renderer/render2d.h>
#include <engine/math/math.h>
#include <engine/debug/debug_system.h>

#include <constants.h>
#include <debug/FeatureTest.h>
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
	namespace {
		int32_t FindEntityLayerUid(const sc::world::Level& level) {
			for (const auto& layer : level.GetLayers()) {
				if (layer->GetType() == sc::world::LayerType::Entity) {
					return layer->GetUid();
				}
			}

			LOG_W("level %s has no entity layer, so anything spawned at runtime won't render", level.GetName());
			return -1;
		}
	}

    void GameplayLayer::OnAttach() {
		// load keybinds
		auto gameplayInput = std::make_shared<input::InputContext>();
		gameplayInput->Bind(Key::W, static_cast<int32_t>(InputAction::MoveUp));
		gameplayInput->Bind(Key::A, static_cast<int32_t>(InputAction::MoveLeft));
		gameplayInput->Bind(Key::S, static_cast<int32_t>(InputAction::MoveDown));
		gameplayInput->Bind(Key::D, static_cast<int32_t>(InputAction::MoveRight));
		gameplayInput->Bind(Key::F2, static_cast<int32_t>(InputAction::SpawnEnt));
		gameplayInput->Bind(Key::F3, static_cast<int32_t>(InputAction::DestroyEnt));
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

		// add feature test module (requires world)
		SC_DEBUG_ONLY(dbg::RegisterModule(std::make_unique<game::debug::FeatureTest>(*m_world, [this](int32_t uid) { SwitchLevel(uid); })));

		// get level
		m_currentLevel = m_world->GetLevel("World_Level_0").get();
		if (!m_currentLevel) {
			LOG_W("failed to get level 0");
			return;
		}

		m_entityFactory.SetCurrentLevel(m_currentLevel);

		// find which layer spawned entities should be in
		m_entityLayerUid = FindEntityLayerUid(*m_currentLevel);

		// apply bounds to camera
		UpdateCameraBounds();

		// get player reference
		const auto view = m_registry.ViewAll<game::components::PlayerTag>();
		for (auto [entityHandle] : view) {
			m_player = sc::ecs::Entity(entityHandle, &m_registry);
			break; // only 1 player
		}

		if (!m_player) {
			LOG_W("no player found !");
		}
    }

    void GameplayLayer::OnFixedUpdate(float timeStep) {
		if (!m_currentLevel) {
			return;
		}

		int32_t levelUid = m_currentLevel->GetUid();

		// handle player keybinds
		systems::UpdatePlayerInput(m_registry, levelUid);

		// calculate every entity movement
		systems::UpdateCharacterMovement(m_registry, levelUid, timeStep);

		// move entities in engine
		ecs::physics_system::UpdateKinematics(m_registry, *m_currentLevel, timeStep);

		// flush destroyed entities
		ecs::lifecycle_system::FlushDestroyed(m_registry, *m_world);
    }

    void GameplayLayer::OnUpdate(float deltaTime) {
		SC_DEBUG_ONLY(dbg::SetContext(dbg::DebugContext{
			.registry = &m_registry,
			.level = m_currentLevel,
			.camera = &m_camera,
			.deltaTime = deltaTime
		}));

		if (!m_player) {
			return;
		}

		if (input::IsActionJustPressed(static_cast<uint32_t>(InputAction::SpawnEnt))) {
			const auto& trans = m_player.GetComponent<ecs::TransformComponent>();
			m_entityFactory.Spawn(entities::EntityType::Dragon, entities::SpawnParams{
				.position = { trans.pos + sf::Vector2f{ 50.f, 50.f } },
				.layerUid = m_entityLayerUid
			});
		}

		// temporary test for entity destruction: F3 destroys the first dragon found
		if (input::IsActionJustPressed(static_cast<uint32_t>(InputAction::DestroyEnt))) {
			for (auto [handle, tag] : m_registry.ViewAll<const ecs::TagComponent>()) {
				if (tag.tag == "Dragon") {
					LOG_I("destroying dragon %u", static_cast<uint32_t>(handle));
					m_registry.QueueDestroy(handle);
					break;
				}
			}
		}
    }

    void GameplayLayer::OnRender(sf::RenderWindow& window, float alpha) {
		if (!m_currentLevel) {
			LOG_W("level was null");
			return;
		}

		// center camera on player
		if (m_player && m_player.HasComponent<ecs::TransformComponent>()) {
			// interpolate camera pos
			const auto& trans = m_player.GetComponent<ecs::TransformComponent>();
			m_camera.SetPosition(sc::math::Lerp(trans.prevPos, trans.pos, alpha));
		}

		ecs::render_system::RenderWorld(m_registry, *m_currentLevel, window, m_camera, alpha);
    }

	void GameplayLayer::SwitchLevel(int32_t levelUid) {
		sc::world::Level* targetLevel = m_world->GetLevel(levelUid).get();
		if (!targetLevel || !m_player) {
			if (!targetLevel) LOG_W("trying to switch to an unknown level: %d", levelUid);
			else if (!m_player) LOG_W("trying to switch to a level but there is no player");
			return;
		}

		m_currentLevel = targetLevel;
		m_entityLayerUid = FindEntityLayerUid(*m_currentLevel);
		m_entityFactory.SetCurrentLevel(m_currentLevel);
		UpdateCameraBounds();
		ecs::lifecycle_system::MoveToLevel(m_registry, *m_world, m_player.GetHandle(), levelUid, { 0.f, 0.f });
	}

	void GameplayLayer::UpdateCameraBounds() {
		m_camera.SetBounds({ 0.f, 0.f }, static_cast<sf::Vector2f>(m_currentLevel->GetSize()));
	}
}
