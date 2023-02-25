#include "depth-color-renderpass.hpp"

namespace geg::vulkan {
	DepthColorRenderpass::DepthColorRenderpass(
			std::shared_ptr<Device> device, std::shared_ptr<Swapchain> swapchain) {
		m_device = device;
		m_swapchain = swapchain;
		m_depth_format = vk::Format::eD32SfloatS8Uint;

		std::array<vk::AttachmentDescription, 2> attachment_descriptions;
		attachment_descriptions[0] = vk::AttachmentDescription{
				.flags = vk::AttachmentDescriptionFlags(),
				.format = swapchain->format(),
				.samples = vk::SampleCountFlagBits::e1,
				.loadOp = vk::AttachmentLoadOp::eClear,
				.storeOp = vk::AttachmentStoreOp::eStore,
				.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
				.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
				.initialLayout = vk::ImageLayout::eUndefined,
				.finalLayout = vk::ImageLayout::eColorAttachmentOptimal,
		};

		attachment_descriptions[1] = vk::AttachmentDescription{
				.flags = vk::AttachmentDescriptionFlags(),
				.format = m_depth_format,
				.samples = vk::SampleCountFlagBits::e1,
				.loadOp = vk::AttachmentLoadOp::eClear,
				.storeOp = vk::AttachmentStoreOp::eDontCare,
				.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
				.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
				.initialLayout = vk::ImageLayout::eUndefined,
				.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
		};

		vk::AttachmentReference color_reference{
				.attachment = 0,
				.layout = vk::ImageLayout::eColorAttachmentOptimal,
		};

		vk::AttachmentReference depth_reference{
				.attachment = 1,
				.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
		};

		vk::SubpassDescription subpass{
				.flags = vk::SubpassDescriptionFlags(),
				.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
				.colorAttachmentCount = 1,
				.pColorAttachments = &color_reference,
				.pDepthStencilAttachment = &depth_reference,
		};

		vk::SubpassDependency dependacy{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = vk::PipelineStageFlags(
						vk::PipelineStageFlagBits::eColorAttachmentOutput |
						vk::PipelineStageFlagBits::eEarlyFragmentTests),
				.dstStageMask = vk::PipelineStageFlags(
						vk::PipelineStageFlagBits::eColorAttachmentOutput |
						vk::PipelineStageFlagBits::eEarlyFragmentTests),
				.srcAccessMask = vk::AccessFlags(0),
				.dstAccessMask = vk::AccessFlags(
						vk::AccessFlagBits::eColorAttachmentWrite |
						vk::AccessFlagBits::eDepthStencilAttachmentWrite),
		};

		render_pass = device->logical_device.createRenderPass({
				.flags = vk::RenderPassCreateFlags(),
				.attachmentCount = static_cast<uint32_t>(attachment_descriptions.size()),
				.pAttachments = attachment_descriptions.data(),
				.subpassCount = 1,
				.pSubpasses = &subpass,
				.dependencyCount = 1,
				.pDependencies = &dependacy,
		});

		create_framebuffers_and_depth();
		GEG_CORE_INFO("Renderpass created");
	}

	void DepthColorRenderpass::create_framebuffers_and_depth() {
		auto image_info = vk::ImageCreateInfo{
				.imageType = vk::ImageType::e2D,
				.format = m_depth_format,
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

		m_depth_image = m_device->allocator.createImageUnique(image_info, alloc_info);
		m_depth_image_view = m_device->logical_device.createImageViewUnique({
				.image = m_depth_image.first.get(),
				.viewType = vk::ImageViewType::e2D,
				.format = m_depth_format,
				.subresourceRange =
						{
								.aspectMask = vk::ImageAspectFlagBits::eDepth,
								.baseMipLevel = 0,
								.levelCount = 1,
								.baseArrayLayer = 0,
								.layerCount = 1,
						},
		});

		m_swapchain_framebuffers.resize(m_swapchain->images().size());

		int i = 0;
		for (auto& img : m_swapchain->images()) {
			std::array<vk::ImageView, 2> attachments = {img.view, m_depth_image_view.get()};

			m_swapchain_framebuffers[i] = m_device->logical_device.createFramebuffer({
					.renderPass = render_pass,
					.attachmentCount = static_cast<uint32_t>(attachments.size()),
					.pAttachments = attachments.data(),
					.width = m_swapchain->extent().width,
					.height = m_swapchain->extent().height,
					.layers = 1,
			});

			i++;
		}
	}

	void DepthColorRenderpass::cleanup_framebuffers() {
		for (auto fb : m_swapchain_framebuffers) {
			m_device->logical_device.destroyFramebuffer(fb);
		}
	}

	void DepthColorRenderpass::begin(vk::CommandBuffer command_buffer, uint32_t image_index) {
		vk::ClearValue color_clear{
				.color =
						{
								.float32 = {{1, 1, 1, 1}},
						},
		};

		vk::ClearValue depth_clear{
				.depthStencil =
						{
								.depth = 1,
								.stencil = 0,
						},
		};

		std::array<vk::ClearValue, 2> clear = {color_clear, depth_clear};

		command_buffer.beginRenderPass(
				vk::RenderPassBeginInfo{
						.renderPass = render_pass,
						.framebuffer = m_swapchain_framebuffers[image_index],
						.renderArea =
								{
										.offset = {0, 0},
										.extent = m_swapchain->extent(),
								},
						.clearValueCount = static_cast<uint32_t>(clear.size()),
						.pClearValues = clear.data(),
				},
				vk::SubpassContents::eInline);
	}

	DepthColorRenderpass::~DepthColorRenderpass() {
		GEG_CORE_WARN("destroying renderpass");
		cleanup_framebuffers();
		m_device->logical_device.destroyRenderPass(render_pass);
	}
}		 // namespace geg::vulkan
