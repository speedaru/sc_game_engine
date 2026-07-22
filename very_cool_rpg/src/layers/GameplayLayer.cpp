#include <engine/ecs/Components.h>
#include <engine/graphics/Texture2D.h>
#include <engine/graphics/render2d.h>

#include <game/constants.h>
#include <game/layers/GameplayLayer.h>
#include <game/utils/render_utils.h>

#include <engine/graphics/SFMLTexture2D.h>

namespace game {
    void GameplayLayer::OnAttach() {
        m_player = m_scene.CreateEntity("Hero");
        m_player.AddComponent<ecs::TransformComponent>(sf::Vector2f(200.f, 200.f));
        auto playerTexture = sc::graphics::CreateTexture2D(game::ASSETS_DIR / "knight.png");
        m_player.AddComponent<ecs::SpriteComponent>(playerTexture);

        ecs::Entity dragon = m_scene.CreateEntity("dragon");
        dragon.AddComponent<ecs::TransformComponent>(sf::Vector2f(300.f, 200.f));
        auto dragonTexture = sc::graphics::CreateTexture2D(game::ASSETS_DIR / "dragon.png");
        dragon.AddComponent<ecs::SpriteComponent>(dragonTexture);
    }

    void GameplayLayer::OnFixedUpdate(float timeStep) {
    }

    void GameplayLayer::OnRender(sf::RenderWindow& window) {
        auto view = m_scene.GetRegistry().view<const ecs::TransformComponent, ecs::SpriteComponent>();

        sc::graphics::render2d::BeginScene();

        for (auto [entityHandle, trans, spr] : view.each()) {
            sc::ecs::Entity entity(entityHandle, &m_scene);
            render_utils::SubmitEntity(entity);
        }

        sc::graphics::render2d::EndScene(window);
    }
}
