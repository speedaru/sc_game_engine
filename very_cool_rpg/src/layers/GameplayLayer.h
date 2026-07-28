#pragma once
#include <engine/core/ILayer.h>
#include <engine/core/Input.h>
#include <engine/ecs/Scene.h>
#include <engine/ecs/Entity.h>
#include <engine/graphics/Camera2D.h>
#include <engine/graphics/Level.h>

#include <factories/EntityFactory.h>
#include <constants.h>

namespace game {
	class GameplayLayer : public sc::core::ILayer {
	public:
		void OnAttach() override;
		void OnFixedUpdate(float timeStep) override;
		void OnRender(sf::RenderWindow& window) override;

	private:
		// core engine stuff
		sc::ecs::Scene m_scene = sc::ecs::Scene("GameplayLayer Scene");
		sc::graphics::Camera2D m_camera{ NATIVE_WIDTH, NATIVE_HEIGHT };

		// entities
		sc::ecs::Entity m_player;
		factories::EntityFactory m_entityFactory;

		// world
		std::shared_ptr<sc::graphics::Level> m_level;
	};
}
