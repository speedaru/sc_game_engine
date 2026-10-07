#pragma once
#include <string>
#include <tuple>
#include <vector>
#include <utility>
#include <ranges>
#include <cstdint>

#include <entt/entt.hpp>

#include <engine/ecs/Components.h>
#include <engine/utils/logging.h>

// TODO: remove all entt:: usage in solution outside of engine/ecs/Entity.h and Registry.h

namespace sc::ecs {
	class Entity;
	using EntityHandle = entt::entity;

	class Registry {
		ADD_CLASS_TAG("Unnamed Registry");
	public:
		Registry(const char* tag);
		Registry() : Registry(DEFAULT_TAG) {}

		~Registry();

		Entity CreateEntity(const std::string& name = "Unnamed Entity");

		// entity lifetime

		void QueueDestroy(EntityHandle handle);

		// move queue out and leave it empty. only lifecycle_system should call this
		std::vector<EntityHandle> TakePendingDestroy();

		// destroys right away and skips the queue. only lifecycle_system should call this
		void DestroyNow(EntityHandle handle);

		bool IsEntityValid(EntityHandle handle) const;

		// component access

		template <typename T, typename... Args>
		decltype(auto) AddComponent(EntityHandle handle, Args&&... args) {
			return m_registry.emplace<T>(handle, std::forward<Args>(args)...);
		}

		template <typename T>
		T& GetComponent(EntityHandle handle) { return m_registry.get<T>(handle); }

		template <typename T>
		const T& GetComponent(EntityHandle handle) const { return m_registry.get<T>(handle); }

		template <typename T>
		T* TryGetComponent(EntityHandle handle) { return m_registry.try_get<T>(handle); }

		template <typename T>
		const T* TryGetComponent(EntityHandle handle) const { return m_registry.try_get<T>(handle); }

		template <typename T>
		bool HasComponent(EntityHandle handle) const { return m_registry.any_of<T>(handle); }

		// iteration API. each returns a lazy range of tuple<EntityHandle, Components&...>
		// empty components (tags) still filter the view but are left out of the tuple
		// the const overloads need const component types

		// every entity in every level, so only use this for genuinely global queries
		template <typename... RequiredComponents>
		auto ViewAll() { return ViewAllImpl<RequiredComponents...>(*this); }

		template <typename... RequiredComponents>
		auto ViewAll() const { return ViewAllImpl<RequiredComponents...>(*this); }

		// only entities that belong to the given level
		template <typename... RequiredComponents>
		auto ViewLevel(int32_t levelUid) { return ViewLevelImpl<RequiredComponents...>(*this, levelUid); }

		template <typename... RequiredComponents>
		auto ViewLevel(int32_t levelUid) const { return ViewLevelImpl<RequiredComponents...>(*this, levelUid); }

		// predicate receives (EntityHandle, RequiredComponents&...) minus any empty ones
		template <typename... RequiredComponents, typename Predicate>
		auto ViewCustom(Predicate&& pred) {
			return ViewCustomImpl<RequiredComponents...>(*this, std::forward<Predicate>(pred));
		}

		template <typename... RequiredComponents, typename Predicate>
		auto ViewCustom(Predicate&& pred) const {
			return ViewCustomImpl<RequiredComponents...>(*this, std::forward<Predicate>(pred));
		}

	private:
		template <typename... RequiredComponents, typename Self>
		static auto MakeView(Self& self) {
			auto view = self.m_registry.template view<RequiredComponents...>();
			static_assert(std::ranges::view<decltype(view)>, "entt view must be a std::ranges::view or the returned range dangles");
			return view;
		}

		template <typename... RequiredComponents, typename Self>
		static auto ViewAllImpl(Self& self) {
			auto view = MakeView<RequiredComponents...>(self);
			return view | std::views::transform(MakeTransformFn<RequiredComponents...>(view));
		}

		template <typename... RequiredComponents, typename Self>
		static auto ViewLevelImpl(Self& self, int32_t levelUid) {
			// level component is part of the view so the filter can read it, but not of the tuple
			auto view = MakeView<const LevelComponent, RequiredComponents...>(self);

			auto filter_fn = [levelUid, view](EntityHandle entity) {
				return view.template get<const LevelComponent>(entity).uid == levelUid;
			};

			return view
				| std::views::filter(filter_fn)
				| std::views::transform(MakeTransformFn<RequiredComponents...>(view));
		}

		template <typename... RequiredComponents, typename Self, typename Predicate>
		static auto ViewCustomImpl(Self& self, Predicate&& pred) {
			auto view = MakeView<RequiredComponents...>(self);

			auto filter_fn = [pred = std::forward<Predicate>(pred)](const auto& tuple) {
				return std::apply(pred, tuple);
			};

			// transform first so the predicate receives the unpacked tuple
			return view
				| std::views::transform(MakeTransformFn<RequiredComponents...>(view))
				| std::views::filter(filter_fn);
		}

		template <typename... RequiredComponents, typename View>
		static auto MakeTransformFn(View view) {
			return [view](EntityHandle entity) {
				// store entity by value, and RequiredComponents by reference.
				// empty components (tags) filter the view but have no data, so they're left out of the tuple
				return std::tuple_cat(
					std::make_tuple(entity),
					MakeComponentRef<RequiredComponents>(view, entity)...
				);
			};
		}

		// entt has no storage for empty types so get<T>() doesn't exist for them
		template <typename T, typename View>
		static auto MakeComponentRef(const View& view, EntityHandle entity) {
			if constexpr (std::is_empty_v<T>) {
				return std::tuple<>();
			}
			else {
				return std::tuple<T&>(view.template get<T>(entity));
			}
		}

	private:
		entt::registry m_registry;
		std::vector<EntityHandle> m_pendingDestroy;
	};
}
