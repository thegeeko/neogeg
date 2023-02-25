#pragma once

#include "geg-vulkan.hpp"
#include "device.hpp"
#include "shader.hpp"
#include "depth-color-renderpass.hpp"

namespace geg::vulkan {
	class GraphicsPipeline {
	public:
		GraphicsPipeline(
				std::shared_ptr<Device> device,
				std::shared_ptr<DepthColorRenderpass> renderpass,
				const Shader& shader);
		~GraphicsPipeline();

		vk::Pipeline pipeline;
		vk::PipelineLayout pipeline_layout;

	private:
		std::shared_ptr<Device> m_device;
    std::shared_ptr<DepthColorRenderpass> m_renderpass;
	};
}		 // namespace geg::vulkan
