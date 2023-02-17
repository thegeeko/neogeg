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

		void recreate();

		const auto format() const { return m_surface_format.format; }
		const auto images() const { return m_images; }
		const auto extent() const { return m_extent; }

		bool should_recreate(std::pair<uint32_t, uint32_t> curr_dimintaions) const {
			return curr_dimintaions.first != m_extent.width || curr_dimintaions.second != m_extent.height;
		};

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

		std::vector<Image> m_images;
	};

}		 // namespace geg::vulkan
