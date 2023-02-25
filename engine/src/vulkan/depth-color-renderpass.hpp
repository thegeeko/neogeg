#pragma once

#include "geg-vulkan.hpp"
#include "device.hpp"
#include "swapchain.hpp"

namespace geg::vulkan {
	class DepthColorRenderpass {
	public:
		DepthColorRenderpass(std::shared_ptr<Device> device, std::shared_ptr<Swapchain> swapchain);
		DepthColorRenderpass(const DepthColorRenderpass &) = delete;
		DepthColorRenderpass(DepthColorRenderpass &&) = delete;
		DepthColorRenderpass &operator=(const DepthColorRenderpass &) = delete;
		DepthColorRenderpass &operator=(DepthColorRenderpass &&) = delete;
		~DepthColorRenderpass();

		std::pair<vma::UniqueImage, vma::UniqueAllocation> m_depth_image;
		vk::UniqueImageView m_depth_image_view;
		void create_framebuffers_and_depth();
		void cleanup_framebuffers();

		void recreate_resources() {
			cleanup_framebuffers();
			create_framebuffers_and_depth();
		};

		void begin(vk::CommandBuffer command_buffer, uint32_t image_index);

		VkRenderPass render_pass;

	private:
		vk::Format m_depth_format;
		std::shared_ptr<Device> m_device;
		std::shared_ptr<Swapchain> m_swapchain;
		std::vector<vk::Framebuffer> m_swapchain_framebuffers;
	};
}		 // namespace geg::vulkan
