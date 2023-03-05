#include "graphics-context.hpp"

#include "vulkan/renderer.hpp"
#include "vulkan/swapchain.hpp"

namespace geg {
	VulkanContext::VulkanContext(std::shared_ptr<Window> window): m_window(window) {
		m_device = std::make_shared<vulkan::Device>(m_window);
		m_swapchain = std::make_shared<vulkan::Swapchain>(m_window, m_device);
		m_current_dimensions = m_window->dimensions();

		create_depth_resources();

		m_present_semaphore = m_device->vkdevice.createSemaphore(vk::SemaphoreCreateInfo{});
		m_render_semaphore = m_device->vkdevice.createSemaphore(vk::SemaphoreCreateInfo{});
		const auto images_count = m_swapchain->image_count();
		m_swapchain_image_fences.resize(images_count);
		for (auto& fence : m_swapchain_image_fences) {
			fence = m_device->vkdevice.createFence(vk::FenceCreateInfo{
					.flags = vk::FenceCreateFlagBits::eSignaled,
			});
		}

		m_command_buffers = m_device->vkdevice.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
				.commandPool = m_device->command_pool,
				.level = vk::CommandBufferLevel::ePrimary,
				.commandBufferCount = images_count,
		});

		vulkan::DepthResources depth_resources{
				.format = depth_format,
				.image_view = m_depth_image_view.get(),
		};

		m_clear_renderer =
				std::make_unique<vulkan::ClearRenderer>(m_device, m_swapchain, depth_resources);

		m_imgui_renderer = std::make_unique<vulkan::ImguiRenderer>(m_device, m_swapchain);
		m_present_renderer = std::make_unique<vulkan::PresentRenderer>(m_device, m_swapchain);
	}

	VulkanContext::~VulkanContext() {
		m_device->vkdevice.waitIdle();
		m_device->vkdevice.destroySemaphore(m_render_semaphore);
		m_device->vkdevice.destroySemaphore(m_present_semaphore);
		for (const auto& fence : m_swapchain_image_fences)
			m_device->vkdevice.destroyFence(fence);
	}

	void VulkanContext::create_depth_resources() {
		const auto image_info = vk::ImageCreateInfo{
				.imageType = vk::ImageType::e2D,
				.format = depth_format,
				.extent = vk::Extent3D{m_swapchain->extent().width, m_swapchain->extent().height, 1},
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = vk::SampleCountFlagBits::e1,
				.tiling = vk::ImageTiling::eOptimal,
				.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
				.sharingMode = vk::SharingMode::eExclusive,
				.initialLayout = vk::ImageLayout::eUndefined,
		};

		constexpr auto alloc_info = vma::AllocationCreateInfo{
				.flags = vma::AllocationCreateFlagBits::eDedicatedMemory,
				.usage = vma::MemoryUsage::eGpuOnly,
		};

		m_depth_image = m_device->allocator.createImageUnique(image_info, alloc_info);
		m_depth_image_view = m_device->vkdevice.createImageViewUnique({
				.image = m_depth_image.first.get(),
				.viewType = vk::ImageViewType::e2D,
				.format = depth_format,
				.subresourceRange =
						{
								.aspectMask = vk::ImageAspectFlagBits::eDepth,
								.baseMipLevel = 0,
								.levelCount = 1,
								.baseArrayLayer = 0,
								.layerCount = 1,
						},
		});
	}

	void VulkanContext::render(const Camera& camera) {
		if (m_current_dimensions.first == 0 || m_current_dimensions.second == 0) return;

		if (should_resize_swapchain) {
			// @TODO check if u need to recreate the fences swapchain images count might change
			const auto res = m_device->vkdevice.waitForFences(m_swapchain_image_fences, true, UINT64_MAX);
			GEG_CORE_ASSERT(res == vk::Result::eSuccess, "Fence timeout")
			m_device->vkdevice.resetFences(m_swapchain_image_fences);

			m_swapchain->recreate();
			create_depth_resources();
		} else {
			const auto res = m_device->vkdevice.waitForFences(
					m_swapchain_image_fences[m_current_image_index], false, UINT64_MAX);
			GEG_CORE_ASSERT(res == vk::Result::eSuccess, "Fence timeout")
			m_device->vkdevice.resetFences(m_swapchain_image_fences[m_current_image_index]);
		}

		auto res = m_device->vkdevice.acquireNextImageKHR(
				m_swapchain->swapchain, UINT64_MAX, m_present_semaphore);

		if (res.result == vk::Result::eSuboptimalKHR) {
			should_resize_swapchain = true;
			m_current_image_index = res.value;
		} else if (res.result == vk::Result::eSuccess)
			m_current_image_index = res.value;
		else
			GEG_CORE_ASSERT(false, "unkonwn error when aquiring next image on swapchain")

		auto cmd = m_command_buffers[m_current_image_index];
		cmd.begin(vk::CommandBufferBeginInfo{});
		m_clear_renderer->fill_commands(cmd, camera, m_current_image_index);
		m_imgui_renderer->fill_commands(cmd, camera, m_current_image_index);
		m_present_renderer->fill_commands(cmd, camera, m_current_image_index);
		cmd.end();

		vk::PipelineStageFlags pipeflg = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		const vk::SubmitInfo subinfo{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &m_present_semaphore,
				.pWaitDstStageMask = &pipeflg,
				.commandBufferCount = 1,
				.pCommandBuffers = &cmd,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &m_render_semaphore,
		};
		m_device->graphics_queue.submit(subinfo, m_swapchain_image_fences[m_current_image_index]);

		const auto present_res = m_device->graphics_queue.presentKHR(vk::PresentInfoKHR{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &m_render_semaphore,
				.swapchainCount = 1,
				.pSwapchains = &m_swapchain->swapchain,
				.pImageIndices = &m_current_image_index,
		});

		if (present_res == vk::Result::eSuboptimalKHR)
			should_resize_swapchain = true;
		else if (present_res != vk::Result::eSuccess)
			GEG_CORE_ASSERT(false, "unkonwn error when presenting image")

		// render stuff
	}
}		 // namespace geg