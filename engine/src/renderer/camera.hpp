#pragma once

#include "glm/glm.hpp"

namespace geg {
	class CameraPositionerInterface {
	public:
		virtual ~CameraPositionerInterface() = default;
		virtual glm::mat4 view_matrix() const = 0;
		virtual glm::vec3 position() const = 0;

	private:
	};

	class Camera {
	public:
		explicit Camera(CameraPositionerInterface& positioner): m_positioner(positioner) {}
		glm::mat4 view_matrix() const { return m_positioner.view_matrix(); }
		glm::vec3 position() const { return m_positioner.position(); }

	private:
		CameraPositionerInterface& m_positioner;
	};
}		 // namespace geg