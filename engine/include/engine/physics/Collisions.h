#pragma once
#include <cstdint>
#include <vector>

#include <SFML/Graphics/Rect.hpp>

#include <engine/ecs/Registry.h>

namespace sc::physics {
	// no cell index. tiles carry one, entities do not
	inline constexpr uint32_t INVALID_TILE = ~0u;

	enum class ColliderType : uint8_t {
		Unknown,
		Entity,
		Tile
	};

	enum class ContactKind : uint8_t {
		Unknown,
		Block,
		Overlap,
	};

	// level space collision box
	// uses world coordinates instead of cell coordinates, because different layers can have different cell sizes
	struct ColliderRef {
		// world space coordinates
		sf::FloatRect box;

		ColliderType type = ColliderType::Unknown;
		bool trigger = false;

		// access via accessors to assert type
		union {
			ecs::EntityHandle entity;
			uint32_t tileIndex;
		} data;

		ecs::EntityHandle GetEntity() const { assert(type == ColliderType::Entity); return data.entity; }
		uint32_t GetTileIndex() const { assert(type == ColliderType::Tile); return data.tileIndex; }
	};

	using CandidateBuffer = std::vector<ColliderRef>;

	struct Contact {
		ecs::EntityHandle self; // the mover
		ColliderRef other;
		sf::Vector2f normal;
		ContactKind kind = ContactKind::Unknown;
	};

	// reusable buffers for the physics step owned by caller
	struct PhysicsScratch {
		CandidateBuffer candidates;
		std::vector<ecs::EntityHandle> gridHits; // hits in spatial grid
		std::vector<Contact> contacts;
	};
}
