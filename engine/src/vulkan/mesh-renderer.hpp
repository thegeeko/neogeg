#pragma once

#include "assets/asset-manager.hpp"
#include "ecs/scene.hpp"
#include "renderer.hpp"
#include "shader.hpp"
#include "assets/meshes/meshes.hpp"
#include "uniform-buffer.hpp"
#include "glm/gtx/transform.hpp"
#include "texture.hpp"

namespace geg::vulkan {
	class MeshRenderer final: public Renderer {
	public:
		MeshRenderer(
				const std::shared_ptr<Device>& device,
				std::shared_ptr<Swapchain> swapchain,
				DepthResources depth_resources);

		~MeshRenderer() override;

		void fill_commands(
				const vk::CommandBuffer& cmd,
				const Camera& camera,
				uint32_t frame_index,
				Scene* scene,
				AssetManager* asset_manager) override;

		glm::mat4 projection = glm::mat4(1);

	private:
		void init_pipeline();

		struct {
			glm::mat4 proj = glm::mat4(1);
			glm::mat4 view = glm::mat4(1);
			glm::mat4 proj_view = glm::mat4(1);
			glm::vec3 cam_pos = glm::vec3(0);
		} global_data{};

		struct {
			glm::vec3 color = glm::vec4(1);
			float metallic = 1.0f;
			float roughness = 1.0f;
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
