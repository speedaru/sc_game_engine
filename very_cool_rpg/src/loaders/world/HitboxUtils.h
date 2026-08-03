#pragma once
#include <SFML/System/Vector2.hpp>
#include <LDtkLoader/Entity.hpp>
#include <LDtkLoader/Tileset.hpp>
#include <nlohmann/json.hpp>

namespace sc::ecs { class Entity; }

namespace game::world_loader {
	// adds a BoxColliderComponent to `entity`, parsed from a JSON array of {x, y, w, h} rects
	// `origin` is subtracted from every hitbox position (used to align hitboxes to an entity's pivot)
	void AddBoxCollider(const sc::ecs::Entity& entity, const nlohmann::json& hitboxArray, sf::Vector2f origin = { 0.f, 0.f });

	// LDtk entities are drawn using a tile from their tileset; this offsets that tile's rect by its pivot
	// so hitbox coordinates authored relative to the tile line up with the entity's transform origin
	sf::Vector2f CalcPivotOffset(const ldtk::Entity& entity);

	// finds the source tileset tile id behind an entity's displayed icon, so its custom data
	// (authored once per tile in the tileset, not per entity instance) can be looked up and reused
	int GetTileIdFromEntity(const ldtk::Entity& entity, const ldtk::Tileset& tileset);
}
