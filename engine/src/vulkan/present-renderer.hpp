#pragma once

#include "renderer.hpp"

namespace geg::vulkan {
	class PresentRenderer final: public Renderer {
	public:
		PresentRenderer(std::shared_ptr<Device> device, std::shared_ptr<Swapchain> swapchain):
				Renderer(std::move(device), std::move(swapchain), {}, {.should_present = true}) {}

		~PresentRenderer() override = default;

		void fill_commands(
				const vk::CommandBuffer& cmd, const Camera& camera, uint32_t frame_index) override {
			begin(cmd, frame_index);
			cmd.endRenderPass();
		}
	};

}		 // namespace geg::vulkan