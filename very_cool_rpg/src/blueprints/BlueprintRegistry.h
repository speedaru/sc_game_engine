#pragma once
#include <factories/EntityFactory.h>

namespace game::blueprints {
	// the single place every entity type is declared. explicit rather than self
	// registering on purpose - see the note in debug_system.h about static libraries
	// dropping object files nothing references
	void RegisterAll(factories::EntityFactory& factory);
}
