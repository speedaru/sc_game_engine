#pragma once
#include <filesystem>
#include <unordered_map>
#include <memory>

#include <LDtkLoader/Entity.hpp>

#include <engine/ecs/Scene.h>
#include <engine/ecs/Components.h>
#include <engine/graphics/Texture2D.h>

namespace sc::ecs::assemblers {
    using TextureCache = std::unordered_map<std::string, std::shared_ptr<sc::graphics::Texture2D>>;

    void AttachTransform(Entity& entity, const ldtk::Entity& ldtkData);

    void AttachSprite(Entity& entity, const ldtk::Entity& ldtkData, const fs::path& projectDir, int16_t zIndex);
}
