#include <pch.h>
#include <layers/GameplayLayer.h>

#include <LDtkLoader/Project.hpp>

#include <engine/input/Input.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/systems/render_system.h>
#include <engine/graphics/Texture2D.h>
#include <engine/renderer/render2d.h>

#include <constants.h>
#include <components/GameComponents.h>
#include <enums/InputActions.h>
#include <blueprints/BlueprintRegistry.h>
#include <systems/player_input.h>
#include <systems/character_movement.h>

using Key = sf::Keyboard::Key;
namespace core = sc::core;
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
		input::PushContext(gameplayInput);

		fs::path projectDir = fs::absolute(ASSETS_DIR);
		fs::path project = projectDir / "world_test1.ldtk";

		// load entity blueprints
		blueprints::RegisterAll(m_entityFactory, projectDir);

		// load world
		m_world = world_loader::Load(project, m_registry, m_entityFactory, m_tileSetManager);
		if (!m_world) {
			return;
		}

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
		ecs::physics_system::UpdateKinematics(m_registry, timeStep);

		// center camera on player
		if (m_player && m_player.HasComponent<ecs::TransformComponent>()) {
			const auto& trans = m_player.GetComponent<ecs::TransformComponent>();
			sf::Vector2f cameraPos = trans.pos;

			// TODO: limit camera in map edges

			m_camera.SetPosition(cameraPos);
		}
    }

    void GameplayLayer::OnRender(sf::RenderWindow& window) {
		const std::string currentLevel = "World_Level_0";
		auto level = m_world->GetLevel(currentLevel);
		if (!level) {
			LOG_W("failed to get level %s", currentLevel.c_str());
			return;
		}

		ecs::render_system::RenderWorld(m_registry, *level, window, m_camera);

		window.setView(m_camera.GetView());

		//const auto& view = m_registry.GetRegistry().view<const ecs::TransformComponent, const ecs::BoxColliderComponent>();
		const auto& view = m_registry.GetRegistry().view<ecs::SpriteComponent, ecs::TransformComponent, ecs::BoxColliderComponent>();
		for (const auto& [entity, spr, trans, collision] : view.each()) {
			for (const ecs::Hitbox& hitbox : collision.hitboxes) {
				sf::RectangleShape rect(hitbox.size);
				rect.setPosition(trans.pos + hitbox.offset);
				rect.setFillColor(sf::Color(255, 0, 0, 100));

				window.draw(rect);
			}
		}

		window.setView(window.getDefaultView());
    }
}
