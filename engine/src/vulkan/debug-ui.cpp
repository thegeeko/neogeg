#include "debug-ui.hpp"
#include "imgui.h"

namespace geg::vulkan {

	DebugUi::DebugUi(
			std::shared_ptr<Window> window,
			std::shared_ptr<Device> device,
			std::shared_ptr<Swapchain> swapchain) {
		m_device = device;
		m_window = window;
		m_renderpass = std::make_shared<ColorRenderpass>(device, swapchain);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		io.Fonts->AddFontFromFileTTF("assets/fonts/mononoki.ttf", 13);

		init_descriptors_pool();
		ImGui_ImplGlfw_InitForVulkan(window->raw_pointer, true);
		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = device->instance;
		initInfo.PhysicalDevice = device->physical_device;
		initInfo.Device = device->logical_device;
		initInfo.QueueFamily = device->queue_family_index.value();
		initInfo.Queue = device->graphics_queue;
		initInfo.PipelineCache = VK_NULL_HANDLE;
		initInfo.DescriptorPool = m_descriptor_pool;
		initInfo.Allocator = VK_NULL_HANDLE;
		initInfo.MinImageCount = 2;
		initInfo.ImageCount = swapchain->image_count();
		initInfo.CheckVkResultFn = [](VkResult result) {
			GEG_CORE_ASSERT(result == VK_SUCCESS, "imgui vulkan error");
		};

		ImGui_ImplVulkan_Init(&initInfo, m_renderpass->render_pass);

		device->single_time_command([](auto cmdb) { ImGui_ImplVulkan_CreateFontsTexture(cmdb); });
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		m_command_buffer = m_device->logical_device
													 .allocateCommandBuffers({
															 .commandPool = m_device->command_pool,
															 .level = vk::CommandBufferLevel::ePrimary,
															 .commandBufferCount = 1,
													 })
													 .front();

		GEG_CORE_INFO("Debug gui inited");
	}

	vk::CommandBuffer DebugUi::render(uint32_t image_index) {
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(m_curr_dimensions.first, m_curr_dimensions.second);

		ImGui::Render();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		m_command_buffer.begin(vk::CommandBufferBeginInfo{});
		m_renderpass->begin(m_command_buffer, image_index);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_command_buffer);
		m_command_buffer.endRenderPass();
		m_command_buffer.end();

		new_frame();

		return m_command_buffer;
	}

	DebugUi::~DebugUi() {
		m_device->logical_device.destroyDescriptorPool(m_descriptor_pool);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void DebugUi::init_descriptors_pool() {
		VkDescriptorPoolSize poolSizes[] = {
				{VK_DESCRIPTOR_TYPE_SAMPLER, 2000},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000},
				{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 2000},
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2000},
				{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 2000},
				{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 2000},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2000},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 2000},
				{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2000}};

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 2000 * IM_ARRAYSIZE(poolSizes);
		poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
		poolInfo.pPoolSizes = poolSizes;

		auto device = static_cast<VkDevice>(m_device->logical_device);
		VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptor_pool);
		GEG_CORE_ASSERT(result == VK_SUCCESS, "can't create descriptor pool for imgui");
	}
}		 // namespace geg::vulkan
