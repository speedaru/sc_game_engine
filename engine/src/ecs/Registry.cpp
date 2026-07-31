#include <pch.h>
#include <engine/ecs/Registry.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/Entity.h>

namespace sc::ecs {
	Registry::Registry(const char* tag) : m_tag(tag) {
		LOG_OBJ_I("initializing entity registry '%s'", m_tag);
	}

	Registry::~Registry() {
		LOG_OBJ_I("destroying entity registry '%s'", m_tag);
	}

	Entity Registry::CreateEntity(const std::string& name) {
		// create entity
		entt::entity handle = m_registry.create();
		Entity ent(handle, this);

		// automatically always add a tag component
		ent.AddComponent<TagComponent>(TagComponent{
			.tag = name
		});

		return ent;
	}
}
