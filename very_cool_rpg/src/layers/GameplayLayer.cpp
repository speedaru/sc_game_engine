#include <engine/ecs/Components.h>

#include <game/constants.h>
#include <game/layers/GameplayLayer.h>

void GameplayLayer::OnAttach() {
    // initialize game world
    m_player = m_scene.CreateEntity("Hero");
    m_player.AddComponent<ecs::PositionComponent>(sf::Vector2f(100.f, 100.f));

    m_playerTexture = sf::Texture(game::ASSETS_DIR / "knight.png");
    m_player.AddComponent<ecs::SpriteComponent>(sf::Sprite(m_playerTexture));
}

void GameplayLayer::OnFixedUpdate(float timeStep) {
}

void GameplayLayer::OnRender(sf::RenderWindow& window) {
    auto view = m_scene.GetRegistry().view<const ecs::PositionComponent, ecs::SpriteComponent>();

    for (auto [tag, pos, spr] : view.each()) {
        spr.sprite.setPosition(pos.pos);
        window.draw(spr.sprite);
    }
}
