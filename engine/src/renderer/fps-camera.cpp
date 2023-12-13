#include "fps-camera.hpp"

namespace geg {
  void CameraPositioner_FirstPerson::update(
      double dt, const glm::vec2& mouse_pos, bool mouse_pressed) {
    if (mouse_pressed && !m_cam_locked) {
      const glm::vec2 delta = mouse_pos - m_mouse_pos;

      glm::quat yaw_quat = glm::angleAxis(glm::radians(delta.x), glm::vec3(0.0f, 1.0f, 0.0f));
      glm::quat pitch_quat = glm::angleAxis(glm::radians(delta.y), glm::vec3(1.0f, 0.0f, 0.0f));
      m_camera_orientation = glm::normalize(yaw_quat * pitch_quat * m_camera_orientation);
    }

    if (m_cam_locked) {
      m_camera_orientation =
          glm::quat(glm::lookAt(m_camera_pos, m_lock_vec, glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    m_mouse_pos = mouse_pos;

    const glm::mat4 v = glm::mat4_cast(m_camera_orientation);
    const glm::vec3 forward = -glm::vec3(v[0][2], v[1][2], v[2][2]);
    const glm::vec3 right = glm::vec3(v[0][0], v[1][0], v[2][0]);
    const glm::vec3 up = glm::cross(right, forward);

    if (m_cam_locked) {
      m_camera_pos += right * 2.0f * static_cast<float>(dt);
      return;
    }

    glm::vec3 accel(0.0f);
    if (movement.forward) accel += forward;
    if (movement.backward) accel -= forward;
    if (movement.left) accel -= right;
    if (movement.up) accel -= up;
    if (movement.down) accel += up;
    if (movement.right) accel += right;
    if (movement.fast_speed) accel *= fast_coef;

    if (accel == glm::vec3(0)) {
      m_move_speed -= m_move_speed * std::min((1.0f / damping) * static_cast<float>(dt), 1.0f);
    } else {
      m_move_speed += accel * acceleration * static_cast<float>(dt);
      const float _max_speed = movement.fast_speed ? max_speed * fast_coef : max_speed;
      if (glm::length(m_move_speed) > _max_speed)
        m_move_speed = glm::normalize(m_move_speed) * _max_speed;
    }

    m_camera_pos += m_move_speed * static_cast<float>(dt);
  }
}    // namespace geg
