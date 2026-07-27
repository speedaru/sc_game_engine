#include <layers/GameplayLayer.h>

#include <engine/ecs/Components.h>
#include <engine/graphics/Texture2D.h>
#include <engine/graphics/render2d.h>
#include <engine/assets/LDtkLoader.h>

#include <constants.h>
#include <components/SpriteComponent.h>
#include <utils/render_utils.h>
namespace ecs = sc::ecs;
namespace gfx = sc::graphics;
namespace assets = sc::assets;
namespace gcomp = game::components;

namespace game {
    void GameplayLayer::OnAttach() {
		RegisterEntityBlueprints();

		// loader
		assets::LDtkLoader loader;
		loader.LoadProject(ASSETS_DIR / "world_test1.ldtk");

		// load level
		m_level = loader.LoadLevelGraphics("World_Level_0");

		auto entitySpawns = loader.GetEntities("World_Level_0", "Entities");
		for (const auto& spawn : entitySpawns) {
			m_entityFactory.Spawn(m_scene, spawn);
		}
    }

    void GameplayLayer::OnFixedUpdate(float timeStep) {
		if (m_player && m_player.HasComponent<ecs::TransformComponent>()) {
			const auto& transform = m_player.GetComponent<ecs::TransformComponent>();
			m_camera.SetPosition(transform.pos);
		}
    }

    void GameplayLayer::OnRender(sf::RenderWindow& window) {
        auto view = m_scene.GetRegistry().view<const ecs::TransformComponent, gcomp::SpriteComponent>();

        sc::graphics::render2d::BeginScene();

		// render world
        gfx::render2d::SubmitLevel(*m_level);

		// render entities
        for (auto [entityHandle, trans, spr] : view.each()) {
            sc::ecs::Entity entity(entityHandle, &m_scene);
            render_utils::SubmitEntity(entity);
        }

        sc::graphics::render2d::EndScene(window, m_camera);
    }

	void GameplayLayer::RegisterEntityBlueprints() {
		m_entityFactory.Register("Player", [this](ecs::Scene& scene, const assets::EntitySpawnData& data) {
			auto entity = scene.CreateEntity("Player");
			entity.AddComponent<ecs::TransformComponent>(data.position);
			entity.AddComponent<game::components::SpriteComponent>(data.tileSetTexture, game::ZLayer::Gameplay, data.textureRect);

			m_player = entity;

			return entity;
		});

		m_entityFactory.Register("Dragon", [this](ecs::Scene& scene, const assets::EntitySpawnData& data) {
			auto entity = scene.CreateEntity("Dragon");
			entity.AddComponent<ecs::TransformComponent>(data.position);
			entity.AddComponent<game::components::SpriteComponent>(data.tileSetTexture, game::ZLayer::Gameplay, data.textureRect);

			return entity;
		});
	}
}
