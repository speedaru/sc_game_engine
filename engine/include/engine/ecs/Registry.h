#pragma once
#include <entt/entt.hpp>

#include <engine/utils/logging.h>

namespace sc::ecs {
	class Entity;

	class Registry {
		ADD_CLASS_TAG("Unnamed Registry");
	public:
		Registry(const char* tag);
		Registry() : Registry(DEFAULT_TAG) {}

		~Registry();

		Entity CreateEntity(const std::string& name = "Unnamed Entity");

		inline entt::registry& GetRegistry() { return m_registry; }
		inline const entt::registry& GetRegistry() const { return m_registry; }

	private:
		entt::registry m_registry;
	};
}