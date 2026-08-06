#pragma once
#include <engine/debug/debug_config.h>

namespace sc::ecs { class Registry; }
namespace sc::world { class Level; }
namespace sc::graphics { class Camera2D; }

namespace sc::debug {
	// the per frame slice of engine state modules are allowed to read, published once a
	// frame via debug::SetContext by whoever actually owns these (the game's gameplay
	// layer - the engine has no global handle on any of them).
	//
	// kept deliberately small. it carries the things that change frame to frame or
	// level to level and nothing else; a module needing game specific state should take
	// it in its constructor at registration rather than growing this struct
	struct DebugContext {
		ecs::Registry* registry = nullptr;
		world::Level* level = nullptr;
		const graphics::Camera2D* camera = nullptr;
		float deltaTime = 0.f;

		// modules are never dispatched on a frame where this fails, so OnDrawUI and
		// OnDrawOverlay can dereference the pointers above without checking them
		bool IsValid() const { return registry && level && camera; }
	};
}
