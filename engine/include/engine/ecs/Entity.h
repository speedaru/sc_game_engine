#pragma once
#include <cassert>

#include <entt/entt.hpp>

#include <engine/ecs/Scene.h>
#include <engine/utils/logging.h>

namespace sc::ecs {
	class Entity {
	public:
		Entity()
			: m_handle(entt::null), m_scene(nullptr) {}

		Entity(const entt::entity handle, Scene* scene)
			: m_handle(handle), m_scene(scene) {}

		operator bool() const { return m_handle != entt::null; }

		template <typename T, typename... Args>
		T& AddComponent(Args&&... args) const {
			assert((bool)*this && m_scene != nullptr);
			return m_scene->GetRegistry().emplace<T>(m_handle, std::forward<Args>(args)...);
		}

		template <typename T>
		const T& GetComponent() const {
			assert((bool)*this && m_scene != nullptr);
			return m_scene->GetRegistry().get<T>(m_handle);
		}

		template <typename T>
		T& GetComponent() {
			assert((bool)*this && m_scene != nullptr);
			return m_scene->GetRegistry().get<T>(m_handle);
		}

		template <typename T>
		bool HasComponent() const {
			assert((bool)*this && m_scene != nullptr);
			return m_scene->GetRegistry().any_of<T>(m_handle);
		}

	private:
		entt::entity m_handle; // entity handle
		Scene* m_scene;
	};
}
