#pragma once
#include <engine/core/ILayer.h>
#include <engine/ecs/Registry.h>
#include <engine/ecs/Entity.h>
#include <engine/graphics/Camera2D.h>
#include <engine/graphics/TileSetManager.h>
#include <engine/physics/Collisions.h>
#include <engine/world/World.h>

#include <factories/EntityFactory.h>
#include <systems/collision_system.h>
#include <layers/LayerRequest.h>
#include <constants.h>

#include <vector>

namespace game {
	class GameplayLayer : public sc::core::ILayer {
	public:
		void OnAttach() override;
		void OnFixedUpdate(float timeStep) override;
		void OnUpdate(float deltaTime) override;
		void OnRender(sf::RenderWindow& window, float alpha) override;

		void SwitchLevel(int32_t levelUid, sf::Vector2f spawnPos);

	private:
		void RegisterDebugModules();

		// everything that has to change together when the current level changes
		void EnterLevel(sc::world::Level& level);

		bool HasPlayer() const { return m_registry.IsEntityValid(m_player.GetHandle()); }

#pragma region layer_requests
		void Apply(const TeleportRequest& request);

		// applies and clears m_requests
		void HandleLayerRequests();
#pragma endregion layer_requests

	private:
		// core engine stuff
		sc::ecs::Registry m_registry = sc::ecs::Registry ("GameplayLayer registry");
		sc::graphics::Camera2D m_camera{ NATIVE_WIDTH, NATIVE_HEIGHT };
		sc::graphics::TileSetManager m_tileSetManager;
		sc::physics::PhysicsScratch m_physicsScratch;
		collision_system::ContactState m_contactState;

		// filled by the systems during a step and applied at the end of it
		std::vector<LayerRequest> m_requests;

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
