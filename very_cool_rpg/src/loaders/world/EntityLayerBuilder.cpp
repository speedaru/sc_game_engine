#include <pch.h>
#include <loaders/world/EntityLayerBuilder.h>

#include <LDtkLoader/Level.hpp>

#include <engine/ecs/Entity.h>
#include <engine/world/EntityLayer.h>
#include <engine/utils/logging.h>

#include <entities/EntityType.h>
#include <entities/SpawnParams.h>

namespace ecs = sc::ecs;
namespace world = sc::world;

namespace game::world_loader {
	namespace {
		// copies a scalar field as is. a field left empty in ldtk is null and stays out of
		// the bag, so the blueprint sees it the same way as a field that doesn't exist
		template <typename T>
		void CopyField(const ldtk::Entity& ldtkEntity, const std::string& name, entities::FieldBag& bag) {
			const auto& field = ldtkEntity.getField<T>(name);
			if (!field.is_null()) {
				bag.fields[name] = field.value();
			}
		}

		// a reference is resolved here to plain values, so nothing after the loader needs
		// the target to be spawned or its level to be built yet
		void CopyEntityRef(const ldtk::Entity& ldtkEntity, const std::string& name, entities::FieldBag& bag) {
			const auto& field = ldtkEntity.getField<ldtk::EntityRef>(name);
			if (field.is_null()) return;

			const ldtk::EntityRef& ref = field.value();
			const ldtk::Entity* target = ref.operator->();
			if (!target) {
				LOG_W("field '%s' on '%s' references an entity that doesn't exist", name.c_str(), ldtkEntity.getName().c_str());
				return;
			}

			const ldtk::IntPoint& pos = target->getPosition();
			bag.fields[name] = entities::EntityRefValue{
				target->layer->level->uid,
				{ static_cast<float>(pos.x), static_cast<float>(pos.y) }
			};
		}

		// names and types come from ldtk's own field list, so getField can't throw here
		entities::FieldBag ReadFields(const ldtk::Entity& ldtkEntity) {
			entities::FieldBag bag;

			for (const ldtk::FieldDef& def : ldtkEntity.allFields()) {
				switch (def.type) {
					case ldtk::FieldType::Int:
						CopyField<int>(ldtkEntity, def.name, bag);
						break;
					case ldtk::FieldType::Float:
						CopyField<float>(ldtkEntity, def.name, bag);
						break;
					case ldtk::FieldType::Bool:
						CopyField<bool>(ldtkEntity, def.name, bag);
						break;
					case ldtk::FieldType::String:
						CopyField<std::string>(ldtkEntity, def.name, bag);
						break;
					case ldtk::FieldType::EntityRef:
						CopyEntityRef(ldtkEntity, def.name, bag);
						break;
					default:
						LOG_W("field '%s' on '%s' has an unsupported type and was skipped", def.name.c_str(), ldtkEntity.getName().c_str());
						break;
				}
			}

			return bag;
		}
	}

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
			if (ldtkEntity.hasTag("marker")) continue;

			const ldtk::IntPoint pos = ldtkEntity.getPosition();

			ctx.entityFactory.Spawn(
				entities::FromLdtkIdentifier(ldtkEntity.getName()),
				entities::SpawnParams{
					.position = { static_cast<float>(pos.x), static_cast<float>(pos.y) },
					.layerUid = ldtkLayer.getDefUid(),
					.fields = ReadFields(ldtkEntity)
				}
			);
		}

		return entityLayer;
	}
}
