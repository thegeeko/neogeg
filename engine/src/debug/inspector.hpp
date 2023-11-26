#pragma once

#include "assets/asset-manager.hpp"
#include "ecs/scene.hpp"
#include "ecs/entity.hpp"
#include "glm/glm.hpp"
#include "imgui.h"

namespace geg {
  namespace ui {
    void draw_vec3(
        const char* label,
        glm::vec3& vec,
        float step_size = 1.0f,
        float reset_value = 0.0f,
        float title_width = 100.0f);

    void draw_text(
        const char* label,
        const char* text,
        ImVec4 color = {0, 0, 0, 1.0f},
        std::function<void()> after = nullptr,
        float title_width = 100.0f);

    bool draw_text_input(const char* label, char* buffer, size_t size, float title_width = 100.0f);
    void draw_smth(const char* label, std::function<void()> lambda, float title_width = 100.0f);
  }    // namespace ui

  namespace panels {
    void draw_entity(Entity& entity);

    class Scene {
    public:
      Scene(geg::Scene& scene): m_scene(scene) {}
      void draw_panel();

    private:
      geg::Scene& m_scene;
      Entity selected_entity;
    };
  }    // namespace panels
}    // namespace geg
