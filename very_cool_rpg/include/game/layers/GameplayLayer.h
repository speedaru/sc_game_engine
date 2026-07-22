#pragma once
#include <engine/core/ILayer.h>
#include <engine/ecs/Scene.h>
#include <engine/ecs/Entity.h>
namespace ecs = sc::ecs;

namespace game {
	class GameplayLayer : public sc::core::ILayer {
	public:
		void OnAttach() override;
		void OnFixedUpdate(float timeStep) override;
		void OnRender(sf::RenderWindow& window) override;

	private:
		ecs::Scene m_scene = ecs::Scene("GameplayLayer Scene");
		ecs::Entity m_player;
	};
}
