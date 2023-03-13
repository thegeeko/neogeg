#include "swapchain.hpp"

#include <algorithm>

namespace geg::vulkan {
	Swapchain::Swapchain(std::shared_ptr<Window> window, std::shared_ptr<Device> device) {
		m_window = window;
		m_device = device;

		std::vector<vk::SurfaceFormatKHR> formats =
				m_device->physical_device.getSurfaceFormatsKHR(m_device->surface);
		GEG_CORE_ASSERT(!formats.empty(), "No surface formats available");

		if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {
			m_surface_format.format = vk::Format::eB8G8R8A8Unorm;
			m_surface_format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
		} else {
			m_surface_format = formats[0];
		}

		vk::SurfaceCapabilitiesKHR surface_capabilities =
				m_device->physical_device.getSurfaceCapabilitiesKHR(m_device->surface);

		auto [width, height] = m_window->dimensions();
		if (surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
			m_extent.width = std::clamp(
					width,
					surface_capabilities.minImageExtent.width,
					surface_capabilities.maxImageExtent.width);
			m_extent.height = std::clamp(
					height,
					surface_capabilities.minImageExtent.height,
					surface_capabilities.maxImageExtent.height);
		} else {
			m_extent = surface_capabilities.currentExtent;
		}

		m_present_mode = vk::PresentModeKHR::eMailbox;

		m_transform =
				(surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ?
						vk::SurfaceTransformFlagBitsKHR::eIdentity :
						surface_capabilities.currentTransform;

		m_composite_alpha =
				(surface_capabilities.supportedCompositeAlpha &
				 vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ?
						vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
				(surface_capabilities.supportedCompositeAlpha &
				 vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ?
						vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
				(surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ?
						vk::CompositeAlphaFlagBitsKHR::eInherit :
						vk::CompositeAlphaFlagBitsKHR::eOpaque;

		m_min_image_count = std::clamp(
				surface_capabilities.minImageCount + 1,
				surface_capabilities.minImageCount,
				surface_capabilities.maxImageCount);

		create_swapchain();
	}

	void Swapchain::create_swapchain() {
		// this will handle creating the swapchain
		// to allow recreation of the swapchain

		vk::SurfaceCapabilitiesKHR surface_capabilities =
				m_device->physical_device.getSurfaceCapabilitiesKHR(m_device->surface);

		vk::Extent2D new_extent;
		auto [width, height] = m_window->dimensions();
		if (surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
			new_extent.width = std::clamp(
					width,
					surface_capabilities.minImageExtent.width,
					surface_capabilities.maxImageExtent.width);
			new_extent.height = std::clamp(
					height,
					surface_capabilities.minImageExtent.height,
					surface_capabilities.maxImageExtent.height);
		} else {
			new_extent = surface_capabilities.currentExtent;
		}

		m_extent = new_extent;
		auto new_swapchain = m_device->vkdevice.createSwapchainKHR({
				.surface = m_device->surface,
				.minImageCount = m_min_image_count,
				.imageFormat = m_surface_format.format,
				.imageColorSpace = m_surface_format.colorSpace,
				.imageExtent = m_extent,
				.imageArrayLayers = 1,
				.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
				.imageSharingMode = vk::SharingMode::eExclusive,
				.preTransform = m_transform,
				.compositeAlpha = m_composite_alpha,
				.presentMode = m_present_mode,
				.clipped = VK_TRUE,
				.oldSwapchain = swapchain,
		});

		m_device->vkdevice.destroySwapchainKHR(swapchain);
		swapchain = new_swapchain;

		// clear old image views
		if (!m_images.empty())
			for (auto& image : m_images)
				m_device->vkdevice.destroyImageView(image.view);

		std::vector<vk::Image> images = m_device->vkdevice.getSwapchainImagesKHR(swapchain);
		m_images.resize(images.size());		 // resize local images to match swapchain images
		int i = 0;
		for (auto& image : images) {
			vk::ImageView image_view = m_device->vkdevice.createImageView({
					.image = image,
					.viewType = vk::ImageViewType::e2D,
					.format = m_surface_format.format,
					.components =
							{
									.r = vk::ComponentSwizzle::eIdentity,
									.g = vk::ComponentSwizzle::eIdentity,
									.b = vk::ComponentSwizzle::eIdentity,
									.a = vk::ComponentSwizzle::eIdentity,
							},
					.subresourceRange =
							{
									.aspectMask = vk::ImageAspectFlagBits::eColor,
									.baseMipLevel = 0,
									.levelCount = 1,
									.baseArrayLayer = 0,
									.layerCount = 1,
							},
			});

			m_images[i].image = image;
			m_images[i].view = image_view;
			i++;
		}

		GEG_CORE_INFO("created swapchain with w: {}, h: {}", m_extent.width, m_extent.height);
	}

	void Swapchain::recreate(std::optional<vk::PresentModeKHR> present_mode) {
		if (present_mode.has_value()) m_present_mode = present_mode.value();
		create_swapchain();
	}

	Swapchain::~Swapchain() {
		GEG_CORE_WARN("destroying swapchain");
		// clear old image views
		for (auto& image : m_images) {
			m_device->vkdevice.destroyImageView(image.view);
		}
		m_device->vkdevice.destroySwapchainKHR(swapchain);
	}
}		 // namespace geg::vulkan
