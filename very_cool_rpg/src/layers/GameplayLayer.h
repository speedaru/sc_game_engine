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
		void OnUpdate(float deltaTime) override;
		void OnRender(sf::RenderWindow& window, float alpha) override;

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
		sc::world::Level* m_currentLevel = nullptr;

		// which EntityLayer runtime spawns render into. render_system only draws a sprite
		// whose layerUid matches the layer being drawn, so a spawn without this is
		// invisible. resolved from the loaded level for now - once levels have more than
		// one entity layer this wants a Level::FindLayer(name) instead
		int32_t m_entityLayerUid = -1;
	};
}
