#pragma once
#include <cassert>

#include <engine/ecs/Scene.h>
#include <engine/utils/logging.h>

#include <entt/entt.hpp>

namespace sc::ecs {
	class Entity {
	public:
		Entity()
			: m_handle(entt::null), m_scene(nullptr) {}

		Entity(const entt::entity handle, Scene* scene)
			: m_handle(handle), m_scene(scene) {}

		operator bool() { return m_handle != entt::null; }

		template <typename T, typename... Args>
		T& AddComponent(Args&&... args) {
			assert(*this);
			assert(m_scene != nullptr);
			return m_scene->GetRegistry().emplace<T>(m_handle, std::forward<Args>(args)...);
		}

		template <typename T>
		T& GetComponent() {
			assert(*this);
			assert(m_scene != nullptr);
			return m_scene->GetRegistry().get<T>(m_handle);
		}

	private:
		entt::entity m_handle;
		Scene* m_scene;
	};
}
