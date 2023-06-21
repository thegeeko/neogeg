#include "graphics-context.hpp"

#include <utility>

#include "imgui.h"

#include "vulkan/renderer.hpp"
#include "vulkan/swapchain.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace geg {
	VulkanContext::VulkanContext(std::shared_ptr<Window> window): m_window(std::move(window)) {
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
		m_mesh_renderer =
				std::make_unique<vulkan::MeshRenderer>(m_device, m_swapchain, depth_resources);
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

	void VulkanContext::render(const Camera& camera, Scene* scene, AssetManager* asset_manager) {
		if (m_current_dimensions.first == 0 || m_current_dimensions.second == 0) return;

		draw_debug_ui();

		if (m_swapchain->present_mode() != m_debug_ui_settings.present_mode)
			should_resize_swapchain = true;

		if (should_resize_swapchain) {
			m_device->vkdevice.waitIdle();
			m_swapchain->recreate(m_debug_ui_settings.present_mode);
			create_depth_resources();

			const vulkan::DepthResources& new_dpeth = vulkan::DepthResources{
					.format = depth_format,
					.image_view = m_depth_image_view.get(),
			};

			m_clear_renderer->resize(new_dpeth);
			m_mesh_renderer->resize(new_dpeth);
			m_imgui_renderer->resize();
			m_present_renderer->resize();

			should_resize_swapchain = false;
		}

		auto next_img_res = m_device->vkdevice.acquireNextImageKHR(
				m_swapchain->swapchain, UINT64_MAX, m_present_semaphore);

		if (next_img_res.result == vk::Result::eSuboptimalKHR) {
			should_resize_swapchain = true;
			m_current_image_index = next_img_res.value;
		} else if (next_img_res.result == vk::Result::eSuccess)
			m_current_image_index = next_img_res.value;
		else
			GEG_CORE_ASSERT(false, "unknown error when acquiring next image on swapchain")

		const auto res = m_device->vkdevice.waitForFences(
				m_swapchain_image_fences[m_current_image_index], false, UINT64_MAX);
		GEG_CORE_ASSERT(res == vk::Result::eSuccess, "Fence timeout")
		m_device->vkdevice.resetFences(m_swapchain_image_fences[m_current_image_index]);

		m_mesh_renderer->projection = glm::perspective(
				glm::radians(m_debug_ui_settings.fov),
				(float)m_current_dimensions.first / (float)m_current_dimensions.second,
				0.1f,
				100.f);

		auto cmd = m_command_buffers[m_current_image_index];
		cmd.begin(vk::CommandBufferBeginInfo{});

		m_clear_renderer->fill_commands(cmd, camera, m_current_image_index);

		if (m_debug_ui_settings.mesh_renderer) {
			m_mesh_renderer->fill_commands(cmd, camera, m_current_image_index, scene, asset_manager);
		}

		if (m_debug_ui_settings.imgui_renderer)
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

	void VulkanContext::draw_debug_ui() {
		ImGui::Begin("Vulkan Context");

		if (ImGui::CollapsingHeader("Info: "), ImGuiTreeNodeFlags_DefaultOpen) {
			auto props = m_device->physical_device.getProperties();
			ImGui::Text("Device name: %s", props.deviceName.data());
			ImGui::Text("Device type: %s", vk::to_string(props.deviceType).data());
			ImGui::Text(
					"Current dimensions: %d, %d", m_current_dimensions.first, m_current_dimensions.second);
			ImGui::Text("Current image index: %d", m_current_image_index);
		}

		ImGui::Spacing();
		if (ImGui::CollapsingHeader("settings: ", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat("Fov", &m_debug_ui_settings.fov, 0.1f, 0.1f, 180.f);
			if (ImGui::BeginCombo("Present Mode: ", m_debug_ui_settings.present_mode_name.c_str())) {
				if (ImGui::Selectable("Fifo - Vsync")) {
					m_debug_ui_settings.present_mode = vk::PresentModeKHR::eFifo;
					m_debug_ui_settings.present_mode_name = "Fifo - Vsync";
				}
				if (m_debug_ui_settings.present_mode == vk::PresentModeKHR::eFifo)
					ImGui::SetItemDefaultFocus();

				if (ImGui::Selectable("Immediate")) {
					m_debug_ui_settings.present_mode = vk::PresentModeKHR::eImmediate;
					m_debug_ui_settings.present_mode_name = "Immediate";
				}
				if (m_debug_ui_settings.present_mode == vk::PresentModeKHR::eImmediate)
					ImGui::SetItemDefaultFocus();

				if (ImGui::Selectable("Mailbox")) {
					m_debug_ui_settings.present_mode = vk::PresentModeKHR::eMailbox;
					m_debug_ui_settings.present_mode_name = "Mailbox";
				}
				if (m_debug_ui_settings.present_mode == vk::PresentModeKHR::eMailbox)
					ImGui::SetItemDefaultFocus();

				if (ImGui::Selectable("Fifo Relaxed")) {
					m_debug_ui_settings.present_mode = vk::PresentModeKHR::eFifoRelaxed;
					m_debug_ui_settings.present_mode_name = "Fifo Relaxed";
				}
				if (m_debug_ui_settings.present_mode == vk::PresentModeKHR::eFifoRelaxed)
					ImGui::SetItemDefaultFocus();

				ImGui::EndCombo();
			}
			ImGui::Checkbox("ImGui Renderer: ", &m_debug_ui_settings.imgui_renderer);
			ImGui::Checkbox("Mesh Renderer: ", &m_debug_ui_settings.mesh_renderer);
		}
		ImGui::End();
	}
}		 // namespace geg
