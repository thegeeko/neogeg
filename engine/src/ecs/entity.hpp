#pragma once

#include "entt/entt.hpp"
#include "scene.hpp"

namespace geg {
	class Entity {
	public:
		template<typename T>
		bool has_component() const {
			return m_scene.get_reg().any_of<T>(m_handle);
		}

		template<typename T, typename... Args>
		void add_component(Args&&... args) {
			GEG_CORE_ASSERT(!has_component<T>(), "This component already exist");
			m_scene.get_reg().emplace<T>(m_handle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& get_component() {
			GEG_CORE_ASSERT(has_component<T>(), "This component doesn't exist");
			return m_scene.get_reg().get<T>(m_handle);
		}

	private:
		Entity(Scene& scene): m_scene(scene) { m_handle = m_scene.get_reg().create(); }
		Entity(Scene& scene, entt::entity handle): m_scene(scene), m_handle(handle){};

		entt::entity m_handle;
		Scene& m_scene;

		friend class Scene;
	};
}		 // namespace geg
