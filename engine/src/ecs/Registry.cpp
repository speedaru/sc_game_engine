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
		EntityHandle handle = m_registry.create();
		Entity ent(handle, this);

		// automatically always add a tag component
		ent.AddComponent<TagComponent>(TagComponent{
			.tag = name
		});

		return ent;
	}

	void Registry::QueueDestroy(EntityHandle handle) {
		m_pendingDestroy.push_back(handle);
	}

	std::vector<EntityHandle> Registry::TakePendingDestroy() {
		return std::exchange(m_pendingDestroy, {});
	}

	void Registry::DestroyNow(EntityHandle handle) {
		m_registry.destroy(handle);
	}

	bool Registry::IsEntityValid(EntityHandle handle) const {
		return m_registry.valid(handle);
	}
}
