#pragma once
#include <engine/core/ILayer.h>
#include <engine/ecs/Scene.h>
#include <engine/ecs/Entity.h>
#include <engine/ecs/EntityFactory.h>
#include <engine/graphics/Camera2D.h>
#include <engine/graphics/LevelGraphics.h>

namespace game {
	class GameplayLayer : public sc::core::ILayer {
	public:
		void OnAttach() override;
		void OnFixedUpdate(float timeStep) override;
		void OnRender(sf::RenderWindow& window) override;

	private:
		void RegisterEntityBlueprints();

	private:
		// 2d engine stuff
		sc::ecs::Scene m_scene = sc::ecs::Scene("GameplayLayer Scene");
		sc::graphics::Camera2D m_camera{ 640.f, 360.f };

		// entities
		sc::ecs::Entity m_player;
		sc::ecs::EntityFactory m_entityFactory;

		// world
		std::shared_ptr<sc::graphics::LevelGraphics> m_level;
	};
}
