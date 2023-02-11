#pragma once

#include "core/window.hpp"
#include "device.hpp"
#include "geg-vulkan.hpp"

namespace geg::vulkan {
	class Swapchain {
	public:
		Swapchain(std::shared_ptr<Window> window, std::shared_ptr<Device> device);
		~Swapchain();
		Swapchain(const Swapchain &) = delete;
		Swapchain &operator=(const Swapchain &) = delete;
		Swapchain(Swapchain &&) = delete;
		Swapchain &operator=(Swapchain &&) = delete;

    void recreate();

	private:
		void create_swapchain();

		std::shared_ptr<Window> m_window;
		std::shared_ptr<Device> m_device;

		vk::SurfaceFormatKHR m_surface_format;
		vk::PresentModeKHR m_present_mode;
		vk::Extent2D m_extent;
		vk::SurfaceTransformFlagBitsKHR m_transform;
		vk::CompositeAlphaFlagBitsKHR m_composite_alpha;

		vk::SwapchainKHR m_swapchain = nullptr;
		std::vector<Image> m_images;
	};

}		 // namespace geg::vulkan
