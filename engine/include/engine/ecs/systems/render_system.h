#pragma once
#include <SFML/Graphics/RenderWindow.hpp>

#include <engine/ecs/Registry.h>
#include <engine/world/Level.h>
#include <engine/graphics/Camera2D.h>

namespace sc::ecs::render_system  {
    // handles entire rendering pipeline
    void RenderWorld(
        const Registry& registry,
        const sc::world::Level& level,
        sf::RenderWindow& window,
        const sc::graphics::Camera2D& camera
    );
}
