#include <pch.h>
#include <engine/ecs/Scene.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/Entity.h>

namespace sc::ecs {
	Scene::Scene() : m_tag(DEFAULT_TAG) {
		LOG_OBJ_I("initializing scene '%s'", m_tag);
	}

	Scene::Scene(const char* tag) : m_tag(tag) {
		LOG_OBJ_I("initializing scene '%s'", m_tag);
	}

	Scene::~Scene() {
		LOG_OBJ_I("destroying scene '%s'", m_tag);
	}

	Entity Scene::CreateEntity(const std::string& name) {
		// create entity
		entt::entity handle = m_registry.create();
		Entity ent(handle, this);

		// automatically always add a tag component
		ent.AddComponent<TagComponent>(name);

		return ent;
	}
}
