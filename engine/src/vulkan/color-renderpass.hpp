#pragma once

#include "geg-vulkan.hpp"
#include "device.hpp"
#include "swapchain.hpp"

namespace geg::vulkan {
	class ColorRenderpass {
	public:
		ColorRenderpass(std::shared_ptr<Device> device, std::shared_ptr<Swapchain> swapchain);
		ColorRenderpass(const ColorRenderpass &) = delete;
		ColorRenderpass(ColorRenderpass &&) = delete;
		ColorRenderpass &operator=(const ColorRenderpass &) = delete;
		ColorRenderpass &operator=(ColorRenderpass &&) = delete;
		~ColorRenderpass();

		void create_framebuffers();
		void cleanup_framebuffers();

		void recreate_resources() {
			cleanup_framebuffers();
			create_framebuffers();
		};

		void begin(vk::CommandBuffer command_buffer, uint32_t image_index);

		VkRenderPass render_pass;

	private:
		std::shared_ptr<Device> m_device;
		std::shared_ptr<Swapchain> m_swapchain;
		std::vector<vk::Framebuffer> m_swapchain_framebuffers;
	};
}		 // namespace geg::vulkan
