#include "renderpass.hpp"

namespace geg::vulkan {
	Renderpass::Renderpass(std::shared_ptr<Device> device, std::shared_ptr<Swapchain> swapchain) {
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
				.finalLayout = vk::ImageLayout::ePresentSrcKHR,
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
				.pColorAttachments = &color_reference,
				.pDepthStencilAttachment = &depth_reference,
		};

		render_pass = device->logical_device.createRenderPass({
				.flags = vk::RenderPassCreateFlags(),
				.attachmentCount = static_cast<uint32_t>(attachment_descriptions.size()),
				.pAttachments = attachment_descriptions.data(),
				.subpassCount = 1,
				.pSubpasses = &subpass,
		});

		GEG_CORE_INFO("Renderpass created");
	}

	Renderpass::~Renderpass() {
		GEG_CORE_WARN("destroying renderpass");
		m_device->logical_device.destroyRenderPass(render_pass);
	}
}		 // namespace geg::vulkan
