#pragma once

#include "geg-vulkan.hpp"
#include "device.hpp"
#include "swapchain.hpp"

namespace geg::vulkan {
	class Renderpass {
	public:
		Renderpass(std::shared_ptr<Device> device, std::shared_ptr<Swapchain> swapchain);
		Renderpass(const Renderpass &) = delete;
		Renderpass &operator=(const Renderpass &) = delete;
		Renderpass(Renderpass &&) = delete;
		Renderpass &operator=(Renderpass &&) = delete;
		~Renderpass();

		const auto depth_format() const { return m_depth_format; }
		VkRenderPass render_pass;

	private:
		vk::Format m_depth_format;
		std::shared_ptr<Device> m_device;
		std::shared_ptr<Swapchain> m_swapchain;
	};
}		 // namespace geg::vulkan
