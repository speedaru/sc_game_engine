#pragma once
#include <engine/core/ILayer.h>
#include <engine/ecs/Scene.h>
#include <engine/ecs/Entity.h>

namespace game {
	class GameplayLayer : public sc::core::ILayer {
	public:
		void OnAttach() override;
		void OnFixedUpdate(float timeStep) override;
		void OnRender(sf::RenderWindow& window) override;

	private:
		sc::ecs::Scene m_scene = sc::ecs::Scene("GameplayLayer Scene");
		sc::ecs::Entity m_player;
	};
}
