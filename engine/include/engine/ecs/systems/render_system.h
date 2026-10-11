#pragma once

namespace sf {
	class RenderWindow;
}

namespace sc::ecs {
	class Registry;
}

namespace sc::graphics {
	class Camera2D;
}

namespace sc::world {
	class Level;
}

namespace sc::ecs::render_system  {
    // handles entire rendering pipeline
    void RenderWorld(
        const Registry& registry,
        const sc::world::Level& level,
        sf::RenderWindow& window,
        const sc::graphics::Camera2D& camera,
        float alpha
    );
}
