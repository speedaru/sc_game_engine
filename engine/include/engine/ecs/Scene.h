#pragma once
#include <engine/utils/logging.h>

#include <entt/entt.hpp>

namespace sc::ecs {
	class Entity;

	class Scene {
		ADD_CLASS_TAG("Unnamed Scene");
	public:
		Scene();
		Scene(const char* tag);

		~Scene();

		Entity CreateEntity(const std::string& name = "Unnamed Entity");

		inline entt::registry& GetRegistry() { return m_registry; }

	private:
		entt::registry m_registry;
	};
}