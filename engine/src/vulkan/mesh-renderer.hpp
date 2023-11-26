#pragma once

#include "assets/asset-manager.hpp"
#include "ecs/scene.hpp"
#include "shader.hpp"
#include "assets/meshes/meshes.hpp"
#include "uniform-buffer.hpp"
#include "glm/gtx/transform.hpp"
#include "texture.hpp"
#include "renderer/camera.hpp"

namespace geg::vulkan {
	class MeshRenderer {
	public:
		MeshRenderer(
				const std::shared_ptr<Device>& device, vk::Format img_fomrat);

		~MeshRenderer();

		void fill_commands(
				const vk::CommandBuffer& cmd,
				const Camera& camera,
				Scene* scene,
                const Image& color_target,
                const Image& depth_target);

		glm::mat4 projection = glm::mat4(1);

	private:
		std::shared_ptr<Device> m_device;
		void init_pipeline(vk::Format img_format);

		struct Light {
			glm::vec4 pos{0};
			glm::vec4 color{0};
		};

		struct {
			glm::mat4 proj = glm::mat4(1);
			glm::mat4 view = glm::mat4(1);
			glm::mat4 proj_view = glm::mat4(1);
			glm::vec3 cam_pos = glm::vec3(0);
			uint32_t lights_count = 0;
			Light lights[100];
		} global_data{};

		struct {
			uint32_t albedo_only = false;
			uint32_t metallic_only = false;
			uint32_t roughness_only = false;
			float ao = 1.0f;
		} objec_data{};

		struct {
			glm::mat4 model =
					glm::rotate(glm::scale(glm::mat4(1), {0.03, 0.03, 0.03}), 3.140f, {1.0f, 0.0f, 1.0f});
			glm::mat4 norm =
					glm::rotate(glm::scale(glm::mat4(1), {33.3, 33.3, 33.3}), 3.140f, {1.0f, 0.0f, 1.0f});
		} push_data{};

		UniformBuffer m_global_ubo{m_device, sizeof(global_data), 1};
		UniformBuffer m_object_ubo{m_device, sizeof(objec_data), 1};

		vk::Pipeline m_pipeline;
		vk::PipelineLayout m_pipeline_layout;
		Shader m_shader = Shader(m_device, "assets/shaders/pbr.glsl", "pbr");
	};

}		 // namespace geg::vulkan
