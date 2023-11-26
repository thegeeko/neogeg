#pragma once

#include "entt/entt.hpp"
#include "scene.hpp"

namespace geg {
  class Entity {
  public:
    Entity() = default;
    template<typename T>
    bool has_component() const {
      if (m_handle == entt::null) return false;

      return m_scene->get_reg().any_of<T>(m_handle);
    }

    template<typename T, typename... Args>
    void add_component(Args&&... args) {
      GEG_CORE_ASSERT(!has_component<T>(), "This component already exist");
      m_scene->get_reg().emplace<T>(m_handle, std::forward<Args>(args)...);
    }

    template<typename T>
    T& get_component() {
      GEG_CORE_ASSERT(has_component<T>(), "This component doesn't exist");
      return m_scene->get_reg().get<T>(m_handle);
    }

    operator bool() const { return m_handle != entt::null && m_scene; }
    operator uint32_t() const { return (uint32_t)m_handle; }
    operator entt::entity() const { return m_handle; }

    bool operator==(const Entity& other) const {
      return other.m_handle == m_handle && other.m_scene == m_scene;
    }

    bool operator!=(const Entity& other) const { return !operator==(other); }

  private:
    Entity(Scene* scene): m_scene(scene) { m_handle = m_scene->get_reg().create(); }
    Entity(Scene* scene, entt::entity handle): m_scene(scene), m_handle(handle){};

    entt::entity m_handle = entt::null;
    Scene* m_scene = nullptr;

    friend class Scene;
  };
}    // namespace geg
