#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include <LDtkLoader/Entity.hpp>

#include <engine/ecs/Registry.h>
#include <engine/ecs/Components.h>
#include <engine/graphics/Texture2D.h>

namespace game::utils::assemblers {
    using TextureCache = std::unordered_map<std::string, std::shared_ptr<sc::graphics::Texture2D>>;

    void AttachTransform(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData);

    void AttachSprite(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData, const fs::path& projectDir, TextureCache& cache);
}
