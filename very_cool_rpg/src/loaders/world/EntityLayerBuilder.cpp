#include <pch.h>
#include <loaders/world/EntityLayerBuilder.h>

#include <engine/ecs/Entity.h>
#include <engine/world/EntityLayer.h>

#include <loaders/world/HitboxUtils.h>
#include <loaders/world/CustomData.h>

namespace ecs = sc::ecs;
namespace world = sc::world;

namespace game::world_loader {
	bool EntityLayerBuilder::CanHandle(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>&) const {
		return ldtkLayer.getType() == ldtk::LayerType::Entities;
	}

	std::unique_ptr<world::ILevelLayer> EntityLayerBuilder::Build(const ldtk::Layer& ldtkLayer, const std::unordered_set<std::string>&, LoadContext& ctx) const {
		world::LayerId layerId{ ldtkLayer.getName(), ldtkLayer.getDefUid() };
		auto entityLayer = std::make_unique<world::EntityLayer>(layerId);

		for (const auto& ldtkEntity : ldtkLayer.allEntities()) {
			const auto& tileset = *ldtkEntity.getEntityDef()->tileset;
			int tileId = GetTileIdFromEntity(ldtkEntity, tileset);

			const std::string& customData = tileset.getTileCustomData(tileId);
			ecs::Entity entity = ctx.entityFactory.Spawn(*ctx.currentLevel, ctx.registry, ldtkEntity);

			if (!customData.empty()) {
				sf::Vector2f pivotOffset = CalcPivotOffset(ldtkEntity);
				ApplyCustomData(entity, customData, pivotOffset);
			}
		}

		return entityLayer;
	}
}
