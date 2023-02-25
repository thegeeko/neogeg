#include "color-renderpass.hpp"

namespace geg::vulkan {
	ColorRenderpass::ColorRenderpass(
			std::shared_ptr<Device> device, std::shared_ptr<Swapchain> swapchain) {
		m_device = device;
		m_swapchain = swapchain;

		std::array<vk::AttachmentDescription, 1> attachment_descriptions;
		attachment_descriptions[0] = vk::AttachmentDescription{
				.flags = vk::AttachmentDescriptionFlags(),
				.format = swapchain->format(),
				.samples = vk::SampleCountFlagBits::e1,
				.loadOp = vk::AttachmentLoadOp::eLoad,
				.storeOp = vk::AttachmentStoreOp::eStore,
				.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
				.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
				.initialLayout = vk::ImageLayout::eColorAttachmentOptimal,
				.finalLayout = vk::ImageLayout::ePresentSrcKHR,
		};

		vk::AttachmentReference color_reference{
				.attachment = 0,
				.layout = vk::ImageLayout::eColorAttachmentOptimal,
		};

		vk::SubpassDescription subpass{
				.flags = vk::SubpassDescriptionFlags(),
				.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
				.colorAttachmentCount = 1,
				.pColorAttachments = &color_reference,
				.pDepthStencilAttachment = nullptr,
		};

		vk::SubpassDependency dependacy{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
				.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
				.srcAccessMask = vk::AccessFlags(0),
				.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
		};

		render_pass = device->logical_device.createRenderPass({
				.attachmentCount = static_cast<uint32_t>(attachment_descriptions.size()),
				.pAttachments = attachment_descriptions.data(),
				.subpassCount = 1,
				.pSubpasses = &subpass,
				.dependencyCount = 1,
				.pDependencies = &dependacy,
		});

		create_framebuffers();

		GEG_CORE_INFO("Renderpass created");
	}

	void ColorRenderpass::create_framebuffers() {
		m_swapchain_framebuffers.resize(m_swapchain->images().size());

		int i = 0;
		for (auto& img : m_swapchain->images()) {
			std::array<vk::ImageView, 1> attachments = {img.view};

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

	void ColorRenderpass::begin(vk::CommandBuffer command_buffer, uint32_t image_index) {
		vk::ClearValue color_clear{
				.color =
						{
								.float32 = {{1, 1, 1, 1}},
						},
		};

		std::array<vk::ClearValue, 1> clear = {color_clear};

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

	void ColorRenderpass::cleanup_framebuffers() {
		for (auto fb : m_swapchain_framebuffers) {
			m_device->logical_device.destroyFramebuffer(fb);
		}
	}

	ColorRenderpass::~ColorRenderpass() {
		GEG_CORE_WARN("destroying renderpass");
		m_device->logical_device.destroyRenderPass(render_pass);
	}
}		 // namespace geg::vulkan
