#pragma once
#include <cassert>

#include <entt/entt.hpp>

#include <engine/ecs/Registry.h>
#include <engine/utils/logging.h>

namespace sc::ecs {
	class Entity {
	public:
		Entity()
			: m_handle(entt::null), m_registry(nullptr) {}

		Entity(const entt::entity handle, Registry* registry)
			: m_handle(handle), m_registry(registry) {}

		operator bool() const { return m_handle != entt::null; }

		template <typename T, typename... Args>
		void AddComponent(Args&&... args) const {
			assert((bool)*this && m_registry != nullptr);
			m_registry->GetRegistry().emplace<T>(m_handle, std::forward<Args>(args)...);
		}

		template <typename T>
		const T& GetComponent() const {
			assert((bool)*this && m_registry != nullptr);
			return m_registry->GetRegistry().get<T>(m_handle);
		}

		template <typename T>
		T& GetComponent() {
			assert((bool)*this && m_registry != nullptr);
			return m_registry->GetRegistry().get<T>(m_handle);
		}

		template <typename T>
		bool HasComponent() const {
			assert((bool)*this && m_registry != nullptr);
			return m_registry->GetRegistry().any_of<T>(m_handle);
		}

	private:
		entt::entity m_handle; // entity handle
		Registry* m_registry;
	};
}
