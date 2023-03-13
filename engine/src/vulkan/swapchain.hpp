#pragma once

#include <cstdint>
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

		void recreate(std::optional<vk::PresentModeKHR> present_mode);
		vk::PresentModeKHR present_mode() const { return m_present_mode; }

		auto format() const { return m_surface_format.format; }
		auto images() const { return m_images; }
		auto extent() const { return m_extent; }
		uint32_t image_count() const { return m_images.size(); }

		vk::SwapchainKHR swapchain = nullptr;
	private:
		void create_swapchain();

		std::shared_ptr<Window> m_window;
		std::shared_ptr<Device> m_device;

		vk::CompositeAlphaFlagBitsKHR m_composite_alpha;
		vk::SurfaceTransformFlagBitsKHR m_transform;
		vk::SurfaceFormatKHR m_surface_format;
		vk::PresentModeKHR m_present_mode;
		vk::Extent2D m_extent;

		uint32_t m_min_image_count = 0;

		std::vector<Image> m_images;
	};

}		 // namespace geg::vulkan
