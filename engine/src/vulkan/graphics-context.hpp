#pragma once

#include "assets/asset-manager.hpp"
#include "clear-renderer.hpp"
#include "imgui-renderer.hpp"
#include "present-renderer.hpp"
#include "core/window.hpp"
#include "events/events.hpp"
#include "core/input.hpp"
#include "renderer/camera.hpp"

#include "vulkan/device.hpp"
#include "vulkan/swapchain.hpp"
#include "mesh-renderer.hpp"
#include "ecs/scene.hpp"

namespace geg {
	class VulkanContext {
	public:
		VulkanContext(std::shared_ptr<Window> window);
		~VulkanContext();

		void render(const Camera& camera, Scene* scene, AssetManager* asset_man);
		bool resize(const WindowResizeEvent& new_dim) {
			m_current_dimensions = {new_dim.width(), new_dim.height()};
			should_resize_swapchain = true;

			return false;
		};

		bool toggle_ui(const KeyPressedEvent& event) {
			// ` key
			if (event.key_code() == input::KEY_GRAVE_ACCENT) {
				m_debug_ui_settings.imgui_renderer = !m_debug_ui_settings.imgui_renderer;
			}

			return false;
		}

		// @TODO fix hard-coding depth format
		const vk::Format depth_format = vk::Format::eD32SfloatS8Uint;

		std::shared_ptr<vulkan::Device> get_rendering_device() { return m_device; };

	private:
		void create_depth_resources();
		void draw_debug_ui();

		struct {
			bool imgui_renderer = true;
			bool mesh_renderer = true;
			vk::PresentModeKHR present_mode = vk::PresentModeKHR::eMailbox;
			std::string present_mode_name = "Fifo - VSync";
			float fov = 45.0f;
		} m_debug_ui_settings = {};

		std::shared_ptr<Window> m_window;

		std::shared_ptr<vulkan::Device> m_device;
		std::shared_ptr<vulkan::Swapchain> m_swapchain;
		std::pair<uint32_t, uint32_t> m_current_dimensions;
		uint32_t m_current_image_index = 0;

		std::pair<vma::UniqueImage, vma::UniqueAllocation> m_depth_image;
		vk::UniqueImageView m_depth_image_view;

		std::unique_ptr<vulkan::ClearRenderer> m_clear_renderer;
		std::unique_ptr<vulkan::MeshRenderer> m_mesh_renderer;
		std::unique_ptr<vulkan::ImguiRenderer> m_imgui_renderer;
		std::unique_ptr<vulkan::PresentRenderer> m_present_renderer;

		vk::Semaphore m_present_semaphore;
		vk::Semaphore m_render_semaphore;
		std::vector<vk::Fence> m_swapchain_image_fences;
		std::vector<vk::CommandBuffer> m_command_buffers;

		bool should_resize_swapchain = false;
	};
}		 // namespace geg
