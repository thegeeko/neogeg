#pragma once

#include "renderer.hpp"
#include "shader.hpp"
#include "assets/meshes/meshes.hpp"

namespace geg::vulkan {
	class MeshRenderer final: public Renderer {
	public:
		MeshRenderer(
				std::shared_ptr<Device> device,
				std::shared_ptr<Swapchain> swapchain,
				DepthResources depth_resources);

		~MeshRenderer() override;

		void fill_commands(
				const vk::CommandBuffer& cmd, const Camera& camera, uint32_t frame_index) override;

		glm::mat4 projection = glm::mat4(1);

	private:
		void init_pipeline();

		// @TODO: remove this
		Mesh* m = new Mesh("assets/meshes/teapot.gltf", m_device);
		struct {
			glm::vec4 color = glm::vec4(0.6f, 0.4f, 0.1f, 1.f);
			glm::mat4 mvp = glm::mat4(1);
		} push{};

		vk::Pipeline m_pipeline;
		vk::PipelineLayout m_pipeline_layout;
		Shader m_shader = Shader(m_device, "assets/shaders/flat.glsl", "mesh");
	};

}		 // namespace geg::vulkan