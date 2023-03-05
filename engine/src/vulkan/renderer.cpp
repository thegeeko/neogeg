#include "renderer.hpp"

#include "vulkan/renderpass.hpp"

namespace geg::vulkan {
	Renderer::Renderer(
			const std::shared_ptr<Device> device,
			std::shared_ptr<Swapchain> swapchain,
			std::optional<DepthResources> depth_resources,
			RendererInfo info):
			m_device(std::move(device)),
			m_swapchain(std::move(swapchain)), m_has_depth(depth_resources.has_value()),
			m_should_clear(info.should_clear), m_should_present(info.should_present) {
		const auto color_format = m_swapchain->format();

		if (m_has_depth) { m_depth_resources = depth_resources; }

		const RenderpassInfo renderpass_info{
				.has_depth = m_has_depth,
				.m_should_clear = m_should_clear,
				.should_present = m_should_present,
				.depth_format = m_has_depth ? depth_resources.value().format : vk::Format::eUndefined,
				.color_format = color_format,
		};
		m_renderpass = create_renderpass(device, renderpass_info);

		create_framebuffers();
	}

	Renderer::~Renderer() {
		cleanup_framebuffers();
		m_device->vkdevice.destroyRenderPass(m_renderpass);
	}

	void Renderer::create_framebuffers() {
		for (auto& [_, image_view] : m_swapchain->images()) {
			std::vector<vk::ImageView> attachments = {image_view};
			if (m_has_depth) { attachments.push_back(m_depth_resources.value().image_view); }
			vk::FramebufferCreateInfo framebuffer_info = {};
			framebuffer_info.renderPass = m_renderpass;
			framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = m_swapchain->extent().width;
			framebuffer_info.height = m_swapchain->extent().height;
			framebuffer_info.layers = 1;
			m_framebuffers.push_back(m_device->vkdevice.createFramebuffer(framebuffer_info));
		}
	}

	void Renderer::cleanup_framebuffers() {
		for (auto& fb : m_framebuffers)
			m_device->vkdevice.destroyFramebuffer(fb);
	}

	void Renderer::resize(std::optional<DepthResources> depth_resources) {
		m_depth_resources = depth_resources;
		cleanup_framebuffers();
		create_framebuffers();
	}

	void Renderer::begin(const vk::CommandBuffer& cmd, uint32_t image_index) {
		// @TODO make clear layer does the clearing
		std::vector<vk::ClearValue> clear{};

		if (m_should_clear) {
			clear.push_back({
					.color =
							{
									.float32 = {{1, 1, 1, 1}},
							},
			});

			if (m_has_depth) {
				clear.push_back({
						.depthStencil =
								{
										.depth = 1,
										.stencil = 0,
								},
				});
			}
		}

		cmd.beginRenderPass(
				vk::RenderPassBeginInfo{
						.renderPass = m_renderpass,
						.framebuffer = m_framebuffers[image_index],
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
}		 // namespace geg::vulkan