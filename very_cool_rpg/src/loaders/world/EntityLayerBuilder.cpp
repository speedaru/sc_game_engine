#include <pch.h>
#include <loaders/world/EntityLayerBuilder.h>

#include <engine/ecs/Entity.h>
#include <engine/world/EntityLayer.h>

#include <entities/EntityType.h>
#include <entities/SpawnParams.h>

namespace ecs = sc::ecs;
namespace world = sc::world;

namespace game::world_loader {
	bool EntityLayerBuilder::CanHandle(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>&) const {
		return ldtkLayer.getType() == ldtk::LayerType::Entities;
	}

	std::unique_ptr<world::ILevelLayer> EntityLayerBuilder::Build(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>&, LoadContext& ctx) const {
		world::LayerId layerId{ ldtkLayer.getName(), ldtkLayer.getDefUid() };
		auto entityLayer = std::make_unique<world::EntityLayer>(layerId);

		// this is the whole LDtk -> spawn bridge now. no tileset lookups, no custom data,
		// no pivot maths: all of that was per-type work masquerading as per-instance work,
		// and it moved to EntityDefinitionLoader
		for (const auto& ldtkEntity : ldtkLayer.allEntities()) {
			const ldtk::IntPoint pos = ldtkEntity.getPosition();

			ctx.entityFactory.Spawn(
				entities::FromLdtkIdentifier(ldtkEntity.getName()),
				entities::SpawnParams{
					.position = { static_cast<float>(pos.x), static_cast<float>(pos.y) },
					.layerUid = ldtkLayer.getDefUid()
				}
			);
		}

		return entityLayer;
	}
}
