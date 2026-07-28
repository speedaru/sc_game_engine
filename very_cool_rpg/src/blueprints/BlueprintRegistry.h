#pragma once
#include <factories/EntityFactory.h>
#include <utils/assemblers/LdtkAssemblers.h>

namespace game::blueprints {
	void RegisterAll(factories::EntityFactory& factory, const fs::path& projDir);
}
