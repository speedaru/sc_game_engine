#pragma once
#include <unordered_map>

#include <engine/world/Level.h>

namespace sc::world {
	class World {
	public:
		void AddLevel(const std::shared_ptr<Level>& level);

		std::shared_ptr<Level> GetLevel(const std::string& name);
		std::shared_ptr<Level> GetLevel(const int32_t uid);

	private:
		// keys are level uids
		std::unordered_map<int32_t, std::shared_ptr<Level>> m_levels;
	};
}
