#include <layers/GameplayLayer.h>

#include <engine/ecs/Components.h>
#include <engine/graphics/Texture2D.h>
#include <engine/graphics/render2d.h>

#include <constants.h>
#include <components/SpriteComponent.h>
#include <utils/render_utils.h>
namespace ecs = sc::ecs;
namespace gfx = sc::graphics;
namespace gcomp = game::components;

namespace game {
    void GameplayLayer::OnAttach() {
        m_player = m_scene.CreateEntity("Hero");
        m_player.AddComponent<ecs::TransformComponent>(ecs::TransformComponent{
            .pos = sf::Vector2f(200.f, 200.f)
		});
        auto playerTexture = sc::graphics::CreateTexture2D(game::ASSETS_DIR / "knight.png");
        m_player.AddComponent<gcomp::SpriteComponent>(gcomp::SpriteComponent{
            .texture = playerTexture,
            .zlayer = ZLayer::Gameplay
		});

        ecs::Entity dragon = m_scene.CreateEntity("dragon");
        dragon.AddComponent<ecs::TransformComponent>(ecs::TransformComponent{
			.pos = sf::Vector2f(300.f, 200.f)
        });
        auto dragonTexture = sc::graphics::CreateTexture2D(game::ASSETS_DIR / "dragon.png");
        dragon.AddComponent<gcomp::SpriteComponent>(gcomp::SpriteComponent{
            .texture = dragonTexture,
            .zlayer = ZLayer::Gameplay
        });
    }

    void GameplayLayer::OnFixedUpdate(float timeStep) {
    }

    void GameplayLayer::OnRender(sf::RenderWindow& window) {
        auto view = m_scene.GetRegistry().view<const ecs::TransformComponent, gcomp::SpriteComponent>();

        sc::graphics::render2d::BeginScene();

        for (auto [entityHandle, trans, spr] : view.each()) {
            sc::ecs::Entity entity(entityHandle, &m_scene);
            render_utils::SubmitEntity(entity);
        }

        sc::graphics::render2d::EndScene(window);
    }
}
