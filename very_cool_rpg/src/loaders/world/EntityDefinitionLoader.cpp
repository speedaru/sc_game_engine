#include <pch.h>
#include <loaders/world/EntityDefinitionLoader.h>

#include <engine/graphics/Texture2D.h>
#include <engine/utils/logging.h>

#include <entities/EntityDefinition.h>
#include <factories/EntityFactory.h>
#include <loaders/world/HitboxUtils.h>

namespace gfx = sc::graphics;

namespace game::world_loader {
	namespace {
		// local to this pass: two entity types sharing a tileset should share one texture,
		// but nothing needs this cache once the definitions hold their shared_ptrs
		using TextureCache = std::unordered_map<std::string, std::shared_ptr<gfx::Texture2D>>;

		std::shared_ptr<gfx::Texture2D> GetOrLoadTexture(TextureCache& cache, const fs::path& path) {
			const std::string key = path.string();

			auto it = cache.find(key);
			if (it != cache.end()) return it->second;

			if (!fs::is_regular_file(path)) {
				LOG_W("entity texture path doesn't exist: %s", key.c_str());
				return nullptr;
			}

			auto texture = gfx::CreateTexture2D(key);
			cache[key] = texture;
			return texture;
		}
	}

	void LoadEntityDefinitions(const ldtk::Project& project, factories::EntityFactory& factory, const fs::path& projectDir) {
		TextureCache textureCache;

		for (const auto& ldtkDef : project.getEntityDefs()) {
			const std::string& name = ldtkDef.name;
			entities::EntityType type = static_cast<entities::EntityType>(entities::hash(name));

			entities::EntityDefinition definition;
			definition.type = type; // hash name to get EntityType
			definition.debugName = name; // so we can view the entity name in text while debuging
			definition.pivot = { ldtkDef.pivot.x, ldtkDef.pivot.y };
			definition.textureRect = sf::IntRect{
				{ ldtkDef.texture_rect.x, ldtkDef.texture_rect.y },
				{ ldtkDef.texture_rect.width, ldtkDef.texture_rect.height }
			};

			if (!ldtkDef.tileset) {
				LOG_W("entity definition '%s' has no tileset, so it gets no sprite or hitboxes", name.c_str());
				continue;
			}

			const ldtk::Tileset& tileset = *ldtkDef.tileset;

			definition.texture = GetOrLoadTexture(textureCache, projectDir / tileset.path);

			// hitboxes are defined in custom data of each tile in the tileset
			const int tileId = GetTileIdFromRect(ldtkDef.texture_rect, tileset);
			const std::string& customData = tileset.getTileCustomData(tileId);

			// hitboxes coordinates are relative to tile's top left origin
			// but transform position is relative to it's pivot so we need to shift them
			const sf::Vector2f pivotOffset{
				ldtkDef.pivot.x * ldtkDef.texture_rect.width,
				ldtkDef.pivot.y * ldtkDef.texture_rect.height
			};

			definition.hitboxes = ParseHitboxesFromCustomData(customData, pivotOffset);

			LOG_D("loaded entity definition '%s' with %zu hitboxes", name.c_str(), definition.hitboxes.size() );

			factory.SetDefinition(type, std::move(definition));
		}
	}
}
