#include <pch.h>
#include <loaders/world/HitboxUtils.h>

#include <engine/ecs/Entity.h>
#include <engine/ecs/Components.h>

namespace ecs = sc::ecs;

namespace game::world_loader {
	void AddBoxCollider(const ecs::Entity& entity, const nlohmann::json& hitboxArray, sf::Vector2f origin) {
		auto& collider = entity.AddComponent<ecs::BoxColliderComponent>();

		for (const auto& box : hitboxArray) {
			collider.hitboxes.emplace_back(
				box.value("x", 0.f) - origin.x,
				box.value("y", 0.f) - origin.y,
				box.value("w", 16.f),
				box.value("h", 16.f)
			);
		}
	}

	sf::Vector2f CalcPivotOffset(const ldtk::Entity& entity) {
		const auto& rect = entity.getTextureRect();
		auto pivot = entity.getPivot();
		return { pivot.x * rect.width, pivot.y * rect.height };
	}

	int GetTileIdFromEntity(const ldtk::Entity& entity, const ldtk::Tileset& tileset) {
		const auto rect = entity.getTextureRect();

		const int stride = tileset.tile_size + tileset.spacing;
		const int columns = tileset.texture_size.x / tileset.tile_size;

		return ((rect.y - tileset.padding) / stride) * columns +
			((rect.x - tileset.padding) / stride);
	}
}
