#pragma once
#include <memory>

#include "vulkan/geg-vulkan.hpp"
#include "vulkan/device.hpp"
#include "vulkan/renderpass.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

namespace geg::vulkan {
	class DebugUi {
	public:
		DebugUi(
				std::shared_ptr<Window> window,
				std::shared_ptr<Device> device,
				std::shared_ptr<Renderpass> renderpass,
				std::shared_ptr<Swapchain> swapchain);
		~DebugUi();

		void new_frame() {
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}

		void render(std::pair<uint32_t, uint32_t> curr_dimensions) {
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2(curr_dimensions.first, curr_dimensions.second);

			// Rendering
			ImGui::Render();

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}

	private:
		std::shared_ptr<Window> m_window;
		std::shared_ptr<Device> m_device;
		std::shared_ptr<Renderpass> m_renderpass;
		VkDescriptorPool m_descriptor_pool;
		void init_descriptors_pool();
	};
}		 // namespace geg::vulkan