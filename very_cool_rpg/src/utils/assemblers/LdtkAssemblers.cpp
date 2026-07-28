#include <pch.h>

#include <engine/ecs/Entity.h>
#include <engine/ecs/Components.h>

#include <utils/assemblers/LdtkAssemblers.h>

namespace ecs = sc::ecs;
namespace gfx = sc::graphics;

namespace game::utils::assemblers {
	void AttachTransform(ecs::Entity& entity, const ldtk::Entity& ldtkData) {
		ldtk::IntPoint pos = ldtkData.getPosition();
		ldtk::FloatPoint pivot = ldtkData.getPivot();
		entity.AddComponent<ecs::TransformComponent>(sf::Vector2f((float)pos.x, (float)pos.y), sf::Vector2f(pivot.x, pivot.y));
	}

    void AttachSprite(ecs::Entity& entity, const ldtk::Entity& ldtkData, const fs::path& projectDir, TextureCache& cache, int16_t zIndex) {
        if (ldtkData.getTexturePath().empty()) return;

        std::string fullPath = (projectDir / ldtkData.getTexturePath()).string();

        // use cached if available
        if (cache.find(fullPath) == cache.end()) {
            cache[fullPath] = gfx::CreateTexture2D(fullPath);
        }

        // create new texture
        ldtk::IntRect rect = ldtkData.getTextureRect();
        entity.AddComponent<ecs::SpriteComponent>(
            cache[fullPath],
            zIndex,
            sf::IntRect{ { rect.x, rect.y }, { rect.width, rect.height } }
        );
	}
}