#pragma once

#include "renderer.hpp"

// @TODO refactor this to not be a renderer

namespace geg::vulkan {
	class ClearRenderer final: public Renderer {
	public:
		ClearRenderer(
				std::shared_ptr<Device> device,
				std::shared_ptr<Swapchain> swapchain,
				DepthResources depth_resources):
				Renderer(std::move(device), std::move(swapchain), depth_resources, {.should_clear = true}) {
		}

		~ClearRenderer() override = default;

		void fill_commands(
				const vk::CommandBuffer& cmd, const Camera& camera, uint32_t frame_index) override {
			begin(cmd, frame_index);
			cmd.endRenderPass();
		}
	};

}		 // namespace geg::vulkan