#include <pch.h>
#include <engine/ecs/ComponentAssemblers.h>
#include <engine/ecs/Entity.h>

namespace sc::ecs::assemblers {
    TextureCache s_cache;

	void AttachTransform(Entity& entity, const ldtk::Entity& ldtkData) {
		ldtk::IntPoint pos = ldtkData.getPosition();
		ldtk::FloatPoint pivot = ldtkData.getPivot();
		entity.AddComponent<TransformComponent>(sf::Vector2f((float)pos.x, (float)pos.y), sf::Vector2f(pivot.x, pivot.y));
	}

    void AttachSprite(Entity& entity, const ldtk::Entity& ldtkData, const fs::path& projectDir, int16_t zIndex) {
        if (ldtkData.getTexturePath().empty()) return;

        std::string fullPath = (projectDir / ldtkData.getTexturePath()).string();

        // use cached if available
        if (s_cache.find(fullPath) == s_cache.end()) {
            s_cache[fullPath] = graphics::CreateTexture2D(fullPath);
        }

        // create new texture
        ldtk::IntRect rect = ldtkData.getTextureRect();
        entity.AddComponent<SpriteComponent>(
            s_cache[fullPath],
            zIndex,
            sf::IntRect{ { rect.x, rect.y }, { rect.width, rect.height } }
        );
	}
}