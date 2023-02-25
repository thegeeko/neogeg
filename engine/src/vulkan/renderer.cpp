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
		m_renderpass = std::make_shared<vulkan::DepthColorRenderpass>(m_device, m_swapchain);
		m_debug_ui = std::make_shared<vulkan::DebugUi>(m_window, m_device, m_swapchain);
		m_debug_ui->new_frame();

		tmp_shader =
				std::make_shared<vulkan::Shader>(m_device, fs::path("./assets/shaders/flat.glsl"), "flat");
		tmp_pipeline =
				std::make_shared<vulkan::GraphicsPipeline>(m_device, m_renderpass, *tmp_shader.get());

		m_present_semaphore = m_device->logical_device.createSemaphore(vk::SemaphoreCreateInfo{});
		m_render_semaphore = m_device->logical_device.createSemaphore(vk::SemaphoreCreateInfo{});
		m_render_fence = m_device->logical_device.createFence({
				.flags = vk::FenceCreateFlagBits::eSignaled,
		});

		m_command_buffer = m_device->logical_device
													 .allocateCommandBuffers({
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

		GEG_CORE_WARN("destroying vulkan");
	}

	bool VulkanRenderer::resize(WindowResizeEvent dim) {
		m_current_dimintaions = {dim.width(), dim.height()};

		return false;
	}

	void VulkanRenderer::render(Camera camera) {
		ImGui::ShowMetricsWindow();

		ImGui::Begin("color");
		ImGui::ColorPicker4("color", &push.color[0]);
		ImGui::End();

		push.mvp = glm::perspective(
				45.f, (float)m_current_dimintaions.first / m_current_dimintaions.second, 0.1f, 100.f) * camera.view_matrix();

		if (m_current_dimintaions.first == 0 || m_current_dimintaions.second == 0) { return; }

		m_device->logical_device.waitForFences(m_render_fence, true, UINT64_MAX);
		m_device->logical_device.resetFences(m_render_fence);

		if (m_swapchain->should_recreate(m_current_dimintaions)) {
			m_swapchain->recreate();
			m_renderpass->recreate_resources();
			m_debug_ui->resize(m_current_dimintaions);
		}

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
		m_renderpass->begin(m_command_buffer, m_curr_index);
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
		m_command_buffer.endRenderPass();
		m_command_buffer.end();

		auto debug_cmd = m_debug_ui->render(m_curr_index);

		std::array<vk::CommandBuffer, 2> cmds = {m_command_buffer, debug_cmd};

		vk::PipelineStageFlags pipeflg = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo subinfo{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &m_present_semaphore,
				.pWaitDstStageMask = &pipeflg,
				.commandBufferCount = static_cast<uint32_t>(cmds.size()),
				.pCommandBuffers = cmds.data(),
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
	}
}		 // namespace geg
