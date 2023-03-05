#include "renderpass.hpp"

namespace geg::vulkan {
	vk::RenderPass create_renderpass(std::shared_ptr<Device> device, const RenderpassInfo info) {
		std::vector<vk::AttachmentDescription> attachment_descriptions;
		attachment_descriptions.push_back({
				// color attachment
				.flags = vk::AttachmentDescriptionFlags(),
				.format = info.color_format,
				.samples = vk::SampleCountFlagBits::e1,
				.loadOp = info.m_should_clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
				.storeOp = vk::AttachmentStoreOp::eStore,
				.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
				.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
				.initialLayout = info.m_should_clear ? vk::ImageLayout::eUndefined :
																						 vk::ImageLayout::eColorAttachmentOptimal,
				.finalLayout = info.should_present ? vk::ImageLayout::ePresentSrcKHR :
																						 vk::ImageLayout::eColorAttachmentOptimal,
		});

		if (info.has_depth) {
			attachment_descriptions.push_back({
					// depth attachment
					.flags = vk::AttachmentDescriptionFlags(),
					.format = info.depth_format,
					.samples = vk::SampleCountFlagBits::e1,
					.loadOp = info.m_should_clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
					.storeOp = vk::AttachmentStoreOp::eDontCare,
					.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
					.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
					.initialLayout = info.m_should_clear ? vk::ImageLayout::eUndefined :
																							 vk::ImageLayout::eDepthStencilAttachmentOptimal,
					.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
			});
		}

		vk::AttachmentReference color_ref{
				.attachment = 0,
				.layout = vk::ImageLayout::eColorAttachmentOptimal,
		};
		// won't be attached if info.has_depth is false
		vk::AttachmentReference depth_ref{
				.attachment = 1,
				.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
		};

		vk::SubpassDescription subpass{
				.flags = vk::SubpassDescriptionFlags(),
				.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
				.colorAttachmentCount = 1,
				.pColorAttachments = &color_ref,
				.pDepthStencilAttachment = info.has_depth ? &depth_ref : VK_NULL_HANDLE,
		};

		std::vector<vk::SubpassDependency> deps;
		deps.push_back({
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
				.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
				.srcAccessMask = vk::AccessFlags(0),
				.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
		});
		if (info.has_depth) {
			deps.push_back({
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests,
					.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests,
					.srcAccessMask = vk::AccessFlags(0),
					.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
			});
		}

		return device->vkdevice.createRenderPass({
				.flags = vk::RenderPassCreateFlags(),
				.attachmentCount = static_cast<uint32_t>(attachment_descriptions.size()),
				.pAttachments = attachment_descriptions.data(),
				.subpassCount = 1,
				.pSubpasses = &subpass,
				.dependencyCount = static_cast<uint32_t>(deps.size()),
				.pDependencies = deps.data(),
		});
	}
}		 // namespace geg::vulkan