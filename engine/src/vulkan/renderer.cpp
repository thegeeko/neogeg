#include "vulkan/renderer.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.hpp"

#include "vulkan/device.hpp"

namespace geg {
	VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) {
		GEG_CORE_INFO("init vulkan");

		m_window = window;

		m_device = std::make_shared<vulkan::Device>(m_window);
		m_allocator = vma::createAllocator({
				.physicalDevice = m_device->physical_device,
				.device = m_device->logical_device,
				.instance = m_device->instance,
				.vulkanApiVersion = VK_API_VERSION_1_2,
		});
		m_swapchain = std::make_shared<vulkan::Swapchain>(m_window, m_device);
		m_renderpass = std::make_shared<vulkan::Renderpass>(m_device, m_swapchain);

    GEG_CORE_INFO(fs::current_path().string());
		tmp_shader =
				std::make_shared<vulkan::Shader>(m_device, fs::path("./assets/shaders/flat.glsl"), "flat");
		tmp_pipeline = std::make_shared<vulkan::GraphicsPipeline>(m_device, m_renderpass, *tmp_shader.get());

		create_framebuffers_and_depth();
	}

	VulkanRenderer::~VulkanRenderer() {
		cleanup_framebuffers();
		GEG_CORE_WARN("destroying vulkan");
	}

	void VulkanRenderer::create_framebuffers_and_depth() {
		auto image_info = vk::ImageCreateInfo{
				.imageType = vk::ImageType::e2D,
				.format = m_renderpass->depth_format(),
				.extent = vk::Extent3D{m_swapchain->extent().width, m_swapchain->extent().height, 1},
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = vk::SampleCountFlagBits::e1,
				.tiling = vk::ImageTiling::eOptimal,
				.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
				.sharingMode = vk::SharingMode::eExclusive,
				.initialLayout = vk::ImageLayout::eUndefined,
		};

		auto alloc_info = vma::AllocationCreateInfo{
				.flags = vma::AllocationCreateFlagBits::eDedicatedMemory,
				.usage = vma::MemoryUsage::eGpuOnly,
		};

		m_depth_image = m_allocator.createImageUnique(image_info, alloc_info);
		m_depth_image_view = m_device->logical_device.createImageViewUnique({
				.image = m_depth_image.first.get(),
				.viewType = vk::ImageViewType::e2D,
				.format = m_renderpass->depth_format(),
				.subresourceRange =
						{
								.aspectMask = vk::ImageAspectFlagBits::eDepth,
								.baseMipLevel = 0,
								.levelCount = 1,
								.baseArrayLayer = 0,
								.layerCount = 1,
						},
		});

		m_framebuffers.resize(m_swapchain->images().size());

		int i = 0;
		for (auto& img : m_swapchain->images()) {
			std::array<vk::ImageView, 2> attachments = {img.view, m_depth_image_view.get()};

			m_framebuffers[i] = m_device->logical_device.createFramebuffer({
					.renderPass = m_renderpass->render_pass,
					.attachmentCount = 2,
					.pAttachments = attachments.data(),
					.width = m_swapchain->extent().width,
					.height = m_swapchain->extent().height,
					.layers = 1,
			});

			i++;
		}
	}

	bool VulkanRenderer::resize(WindowResizeEvent dim) {
		m_current_dimintaions = {dim.width(), dim.height()};

		return false;
	}

	void VulkanRenderer::cleanup_framebuffers() {
		for (auto fb : m_framebuffers) {
			m_device->logical_device.destroyFramebuffer(fb);
		}
	}

	void VulkanRenderer::render() {
		if (m_current_dimintaions.first == 0 || m_current_dimintaions.second == 0) { return; }

		if (m_swapchain->should_recreate(m_current_dimintaions)) {
			m_swapchain->recreate();
			cleanup_framebuffers();
			create_framebuffers_and_depth();
		}
	};
}		 // namespace geg
