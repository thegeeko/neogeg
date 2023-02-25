#include "vulkan/renderer.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.hpp"

#include "vulkan/device.hpp"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"

namespace geg {
	VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) {
		GEG_CORE_INFO("init vulkan");

		m_window = window;
		m_current_dimintaions = m_window->dimensions();

		m_device = std::make_shared<vulkan::Device>(m_window);

		m_swapchain = std::make_shared<vulkan::Swapchain>(m_window, m_device);
		m_renderpass = std::make_shared<vulkan::Renderpass>(m_device, m_swapchain);
		m_debug_ui = std::make_shared<vulkan::DebugUi>(m_window, m_device, m_renderpass, m_swapchain);
		m_debug_ui->new_frame();

		tmp_shader =
				std::make_shared<vulkan::Shader>(m_device, fs::path("./assets/shaders/flat.glsl"), "flat");
		tmp_pipeline =
				std::make_shared<vulkan::GraphicsPipeline>(m_device, m_renderpass, *tmp_shader.get());

		create_framebuffers_and_depth();

		m_present_semaphore = m_device->logical_device.createSemaphore(vk::SemaphoreCreateInfo{});
		m_render_semaphore = m_device->logical_device.createSemaphore(vk::SemaphoreCreateInfo{});
		m_render_fence = m_device->logical_device.createFence({
				.flags = vk::FenceCreateFlagBits::eSignaled,
		});

		m_command_buffer = m_device->logical_device
													 .allocateCommandBuffers(vk::CommandBufferAllocateInfo{
															 .commandPool = m_device->command_pool,
															 .level = vk::CommandBufferLevel::ePrimary,
															 .commandBufferCount = 1,
													 })
													 .front();


		m = new vulkan::Mesh("assets/meshes/teapot.gltf", m_device);
	}

	VulkanRenderer::~VulkanRenderer() {
		m_device->logical_device.waitIdle();

		m_device->logical_device.destroySemaphore(m_render_semaphore);
		m_device->logical_device.destroySemaphore(m_present_semaphore);
		m_device->logical_device.destroyFence(m_render_fence);

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

		m_depth_image = m_device->allocator.createImageUnique(image_info, alloc_info);
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

		m_swapchain_framebuffers.resize(m_swapchain->images().size());

		int i = 0;
		for (auto& img : m_swapchain->images()) {
			std::array<vk::ImageView, 2> attachments = {img.view, m_depth_image_view.get()};

			m_swapchain_framebuffers[i] = m_device->logical_device.createFramebuffer({
					.renderPass = m_renderpass->render_pass,
					.attachmentCount = static_cast<uint32_t>(attachments.size()),
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
		for (auto fb : m_swapchain_framebuffers) {
			m_device->logical_device.destroyFramebuffer(fb);
		}
	}

	void VulkanRenderer::render() {
		ImGui::ShowMetricsWindow();

		ImGui::Begin("color");
		ImGui::ColorPicker4("color", &push.color[0]);
		ImGui::End();

		push.mvp = glm::perspective(45.f, (float)m_current_dimintaions.first/m_current_dimintaions.second, 0.1f, 100.f);

		m_debug_ui->render(m_current_dimintaions);

		if (m_current_dimintaions.first == 0 || m_current_dimintaions.second == 0) { return; }

		m_device->logical_device.waitForFences(m_render_fence, true, UINT64_MAX);
		m_device->logical_device.resetFences(m_render_fence);

		if (m_swapchain->should_recreate(m_current_dimintaions)) {
			m_swapchain->recreate();
			cleanup_framebuffers();
			create_framebuffers_and_depth();
		}

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

		auto res = m_device->logical_device.acquireNextImageKHR(
				m_swapchain->swapchain, UINT64_MAX, m_present_semaphore);

		if (res.result != vk::Result::eSuccess) __debugbreak();

		m_curr_index = res.value;

		m_command_buffer.reset();

		vk::Viewport vp{
				.x = 0,
				.y = 0,
				.width = (float)m_swapchain->extent().width,
				.height = (float)m_swapchain->extent().height,
				.minDepth = 0,
				.maxDepth = 1,
		};

		m_command_buffer.begin(vk::CommandBufferBeginInfo{});
		m_command_buffer.beginRenderPass(
				vk::RenderPassBeginInfo{
						.renderPass = m_renderpass->render_pass,
						.framebuffer = m_swapchain_framebuffers[m_curr_index],
						.renderArea =
								{
										.offset = {0, 0},
										.extent = m_swapchain->extent(),
								},
						.clearValueCount = static_cast<uint32_t>(clear.size()),
						.pClearValues = clear.data(),
				},
				vk::SubpassContents::eInline);
		m_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, tmp_pipeline->pipeline);
		m_command_buffer.setViewport(
				0,
				vk::Viewport{
						.x = 0,
						.y = 0,
						.width = (float)m_swapchain->extent().width,
						.height = (float)m_swapchain->extent().height,
						.minDepth = 0,
						.maxDepth = 1,
				});
		m_command_buffer.setScissor(
				0,
				vk::Rect2D{
						.offset = 0,
						.extent = m_swapchain->extent(),
				});
		m_command_buffer.pushConstants(
				tmp_pipeline->pipeline_layout,
				vk::ShaderStageFlagBits::eAllGraphics,
				0,
				sizeof(Push),
				&push);
		m_command_buffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				tmp_pipeline->pipeline_layout,
				0,
				1,
				&m->descriptor_set,
				0,
				nullptr);
		m_command_buffer.draw(m->index_count(), 1, 0, 0);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_command_buffer);
		m_command_buffer.endRenderPass();
		m_command_buffer.end();

		vk::PipelineStageFlags pipeflg = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo subinfo{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &m_present_semaphore,
				.pWaitDstStageMask = &pipeflg,
				.commandBufferCount = 1,
				.pCommandBuffers = &m_command_buffer,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &m_render_semaphore,
		};
		m_device->graphics_queue.submit(subinfo, m_render_fence);

		m_device->graphics_queue.presentKHR(vk::PresentInfoKHR{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &m_render_semaphore,
				.swapchainCount = 1,
				.pSwapchains = &m_swapchain->swapchain,
				.pImageIndices = &m_curr_index,
		});

		m_debug_ui->new_frame();
	}
}		 // namespace geg
