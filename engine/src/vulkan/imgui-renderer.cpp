#include "imgui-renderer.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

namespace geg::vulkan {
	ImguiRenderer::ImguiRenderer(
			std::shared_ptr<Device> device, std::shared_ptr<Swapchain> swapchain):
			Renderer(device, swapchain) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.Fonts->AddFontFromFileTTF("assets/fonts/mononoki.ttf", 13);

		create_descriptor_pool();
		ImGui_ImplGlfw_InitForVulkan(m_device->window->raw_pointer, true);
		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = m_device->instance;
		initInfo.PhysicalDevice = m_device->physical_device;
		initInfo.Device = m_device->vkdevice;
		initInfo.QueueFamily = m_device->queue_family_index.value();
		initInfo.Queue = m_device->graphics_queue;
		initInfo.PipelineCache = VK_NULL_HANDLE;
		initInfo.DescriptorPool = m_descriptor_pool;
		initInfo.Allocator = VK_NULL_HANDLE;
		initInfo.MinImageCount = 2;
		initInfo.ImageCount = m_swapchain->image_count();
		initInfo.CheckVkResultFn = [](VkResult result) {
			GEG_CORE_ASSERT(result == VK_SUCCESS, "imgui vulkan error");
		};

		ImGui_ImplVulkan_Init(&initInfo, m_renderpass);

		device->single_time_command([](auto cmdb) { ImGui_ImplVulkan_CreateFontsTexture(cmdb); });
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		GEG_CORE_INFO("Imgui renderer created");

		// @TODO find a better way to do this
		io.DisplaySize = ImVec2(
				static_cast<float>(m_swapchain->extent().width),
				static_cast<float>(m_swapchain->extent().height));
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
			const vk::CommandBuffer& cmd, const Camera& camera, uint32_t frame_index) {
		begin(cmd, frame_index);
		ImGui::ShowDebugLogWindow();
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(
				static_cast<float>(m_swapchain->extent().width),
				static_cast<float>(m_swapchain->extent().height));

		ImGui::Render();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		cmd.endRenderPass();
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
