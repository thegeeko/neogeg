#pragma once

#include "renderer/camera.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "imgui.h"

namespace geg {
  class CameraPositioner_FirstPerson final: public CameraPositionerInterface {
  public:
    CameraPositioner_FirstPerson() = default;
    CameraPositioner_FirstPerson(
        const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up):
        m_camera_pos(pos),
        m_camera_orientation(glm::lookAt(pos, target, up)) {}

    void update(double dt, const glm::vec2& mouse_pos, bool mouse_pressed);

    glm::mat4 view_matrix() const override {
      const glm::mat4 t = glm::translate(glm::mat4(1.0f), -m_camera_pos);
      const glm::mat4 r = glm::mat4_cast(m_camera_orientation);
      return r * t;
    };

    glm::vec3 position() const override { return m_camera_pos; }

    void set_position(const glm::vec3& pos) { m_camera_pos = pos; }
    void reset_mouse_pos(const glm::vec2& p) { m_mouse_pos = p; };

    void draw_debug_ui() {
      ImGui::Begin("camera");
      ImGui::InputFloat3("cam pos", &m_camera_pos.x);
      ImGui::End();
    }

    struct Movement {
      bool forward = false;
      bool backward = false;
      bool left = false;
      bool right = false;
      bool up = false;
      bool down = false;
      bool fast_speed = false;
    } movement;

    float mouse_speed = 0.005f;
    float acceleration = 100.0f;
    float damping = 0.1f;
    float max_speed = 10.0f;
    float fast_coef = 2.0f;

  private:
    glm::vec2 m_mouse_pos = glm::vec2(0);
    glm::vec3 m_camera_pos = glm::vec3(0.0f, 10.0f, 10.0f);
    glm::quat m_camera_orientation = glm::quat(glm::vec3(0));
    glm::vec3 m_move_speed = glm::vec3(0.0f);
  };
}    // namespace geg