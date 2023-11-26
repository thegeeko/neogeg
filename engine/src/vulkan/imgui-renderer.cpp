#include "imgui-renderer.hpp"
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "vulkan/geg-vulkan.hpp"

namespace geg::vulkan {
	ImguiRenderer::ImguiRenderer(
			const std::shared_ptr<Device>& device, vk::Format img_format, uint32_t img_count):
			m_device(device) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.Fonts->AddFontFromFileTTF("assets/fonts/mononoki.ttf", 13);

		create_descriptor_pool();
		ImGui_ImplGlfw_InitForVulkan(m_device->window->raw_pointer, true);
		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.UseDynamicRendering = true;
		initInfo.ColorAttachmentFormat = static_cast<VkFormat>(img_format);
		initInfo.Instance = m_device->instance;
		initInfo.PhysicalDevice = m_device->physical_device;
		initInfo.Device = m_device->vkdevice;
		initInfo.QueueFamily = m_device->queue_family_index.value();
		initInfo.Queue = m_device->graphics_queue;
		initInfo.PipelineCache = VK_NULL_HANDLE;
		initInfo.DescriptorPool = m_descriptor_pool;
		initInfo.Allocator = VK_NULL_HANDLE;
		initInfo.MinImageCount = 2;
		initInfo.ImageCount = img_count;
		initInfo.CheckVkResultFn = [](VkResult result) {
			GEG_CORE_ASSERT(result == VK_SUCCESS, "imgui vulkan error");
		};

		ImGui_ImplVulkan_Init(&initInfo, nullptr);

		ImGui_ImplVulkan_CreateFontsTexture();

		GEG_CORE_INFO("Imgui renderer created");
		io.DisplaySize =
				ImVec2(1920.f, 1080.f);

		// @TODO find a better way to do this
		ImGui::NewFrame();
	}

	ImguiRenderer::~ImguiRenderer() {
		m_device->vkdevice.destroyDescriptorPool(m_descriptor_pool);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		GEG_CORE_WARN("Imgui renderer destroyed");
	}

	void ImguiRenderer::fill_commands(
			const vk::CommandBuffer& cmd, Scene* scene, const Image& target) {
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize =
				ImVec2(static_cast<float>(target.extent.width), static_cast<float>(target.extent.height));

		ImGui::Render();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		vk::RenderingAttachmentInfoKHR attachment_info{};
		attachment_info.imageView = target.view;
		attachment_info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		attachment_info.loadOp = vk::AttachmentLoadOp::eLoad;
		attachment_info.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfoKHR rendering_info{};
		rendering_info.renderArea.extent = target.extent;
		rendering_info.pColorAttachments = &attachment_info;
		rendering_info.colorAttachmentCount = 1;
		rendering_info.layerCount = 1;

		cmd.beginRendering(rendering_info);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		cmd.endRendering();

		ImGui::NewFrame();
	}

	void ImguiRenderer::create_descriptor_pool() {
		constexpr vk::DescriptorPoolSize pool_sizes[] = {
				{vk::DescriptorType::eSampler, 2000},
				{vk::DescriptorType::eCombinedImageSampler, 2000},
				{vk::DescriptorType::eSampledImage, 2000},
				{vk::DescriptorType::eStorageImage, 2000},
				{vk::DescriptorType::eUniformTexelBuffer, 2000},
				{vk::DescriptorType::eStorageTexelBuffer, 2000},
				{vk::DescriptorType::eUniformBuffer, 2000},
				{vk::DescriptorType::eStorageBuffer, 2000},
				{vk::DescriptorType::eUniformBufferDynamic, 2000},
				{vk::DescriptorType::eStorageBufferDynamic, 2000},
				{vk::DescriptorType::eInputAttachment, 2000}};

		const vk::DescriptorPoolCreateInfo pool_info = {
				.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
				.maxSets = 2000 * IM_ARRAYSIZE(pool_sizes),
				.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes)),
				.pPoolSizes = pool_sizes,
		};

		m_descriptor_pool = m_device->vkdevice.createDescriptorPool(pool_info);
	}		 // namespace geg::vulkan
}		 // namespace geg::vulkan
