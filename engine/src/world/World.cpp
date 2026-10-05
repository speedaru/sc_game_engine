#include <pch.h>
#include <engine/world/World.h>

namespace sc::world {
	void World::AddLevel(const std::shared_ptr<Level>& level) {
		m_levels[level->GetId().uid] = level;
	}

	std::shared_ptr<Level> World::GetLevel(const std::string& name) {
		for (const auto& [uid, level] : m_levels) {
			if (level->GetId().name == name) {
				return level;
			}
		}

		return nullptr;
	}

	std::shared_ptr<Level> World::GetLevel(const int32_t uid) {
		auto it = m_levels.find(uid);
		return (it != m_levels.end()) ? it->second : nullptr;
	}
}
