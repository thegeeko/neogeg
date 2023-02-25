#pragma once
#include <memory>

#include "vulkan/geg-vulkan.hpp"
#include "vulkan/device.hpp"
#include "vulkan/color-renderpass.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

namespace geg::vulkan {
	class DebugUi {
	public:
		DebugUi(
				std::shared_ptr<Window> window,
				std::shared_ptr<Device> device,
				std::shared_ptr<Swapchain> swapchain);
		~DebugUi();

		void new_frame() {
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}

		void resize(std::pair<uint32_t, uint32_t> dimensions) {
			m_curr_dimensions = dimensions;
			m_renderpass->recreate_resources();
		}

		vk::CommandBuffer render(uint32_t image_index);

	private:
		std::shared_ptr<Window> m_window;
		std::shared_ptr<Device> m_device;
		std::shared_ptr<ColorRenderpass> m_renderpass;
		vk::CommandBuffer m_command_buffer;
		VkDescriptorPool m_descriptor_pool;
		std::pair<uint32_t, uint32_t> m_curr_dimensions;
		void init_descriptors_pool();
	};
}		 // namespace geg::vulkan