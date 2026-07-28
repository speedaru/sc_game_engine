#pragma once
#include <filesystem>
#include "includes.h"

namespace fs = std::filesystem;

namespace game::blueprints {
	class PlayerBlueprint : public IBlueprint {
	public:
		PlayerBlueprint(const fs::path& projectDir, TextureCache& textureCache)
			: m_projectDir(projectDir), m_textureCache(textureCache) {}

		void Build(sc::ecs::Entity& entity, const ldtk::Entity& ldtkData) const override;

	private:
		fs::path m_projectDir;
		TextureCache& m_textureCache;
	};
}
