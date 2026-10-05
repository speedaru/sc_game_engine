#pragma once
#include <vector>
#include <utility>

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

		void QueueDestroy(entt::entity ent);

		// move queue out and leave it empty
		std::vector<entt::entity> TakePendingDestroy() { return std::exchange(m_pendingDestroy, {}); }

		inline entt::registry& GetRegistry() { return m_registry; }
		inline const entt::registry& GetRegistry() const { return m_registry; }

	private:
		entt::registry m_registry;
		std::vector<entt::entity> m_pendingDestroy;
	};
}