#pragma once
#include <engine/core/ILayer.h>
#include <engine/ecs/Registry.h>
#include <engine/ecs/Entity.h>
#include <engine/graphics/Camera2D.h>
#include <engine/graphics/TileSetManager.h>
#include <engine/world/World.h>

#include <factories/EntityFactory.h>
#include <loaders/world_loader.h>
#include <constants.h>

namespace game {
	class GameplayLayer : public sc::core::ILayer {
	public:
		void OnAttach() override;
		void OnFixedUpdate(float timeStep) override;
		void OnRender(sf::RenderWindow& window) override;

	private:
		// core engine stuff
		sc::ecs::Registry m_registry = sc::ecs::Registry ("GameplayLayer registry");
		sc::graphics::Camera2D m_camera{ NATIVE_WIDTH, NATIVE_HEIGHT };
		sc::graphics::TileSetManager m_tileSetManager;

		// entities
		sc::ecs::Entity m_player;
		factories::EntityFactory m_entityFactory;

		// world
		std::shared_ptr<sc::world::World> m_world;
	};
}
