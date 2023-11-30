#pragma once

#include "assets/asset-manager.hpp"
#include "imgui-renderer.hpp"
#include "core/window.hpp"
#include "events/events.hpp"
#include "core/input.hpp"
#include "renderer/camera.hpp"

#include "vulkan/device.hpp"
#include "vulkan/early-depth-pass.hpp"
#include "vulkan/swapchain.hpp"
#include "mesh-renderer.hpp"
#include "ecs/scene.hpp"

// legit profiler
#include "ImGuiProfilerRenderer.h"
#include "ProfilerTask.h"

namespace geg {
  class VulkanContext {
  public:
    VulkanContext(std::shared_ptr<Window> window);
    ~VulkanContext();

    void render(const Camera& camera, Scene* scene);
    bool resize(const WindowResizeEvent& new_dim) {
      m_current_dimensions = vk::Extent2D{.width = new_dim.width(), .height = new_dim.height()};
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
    void wait_until_free() { m_device->vkdevice.waitIdle(); };

  private:
    void create_depth_resources();
    void draw_debug_ui();

    struct {
      bool imgui_renderer = true;
      bool mesh_renderer = true;
      vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
      std::string present_mode_name = "Fifo - VSync";
      float fov = 45.0f;
    } m_debug_ui_settings = {};

    std::shared_ptr<Window> m_window;

    std::shared_ptr<vulkan::Device> m_device;
    std::shared_ptr<vulkan::Swapchain> m_swapchain;
    vk::Extent2D m_current_dimensions;
    uint32_t m_current_image_index = 0;

    std::pair<vk::Image, VmaAllocation> m_depth_image = {nullptr, nullptr};
    vk::ImageView m_depth_image_view;

    std::unique_ptr<vulkan::DepthPass> m_early_depth_pass;
    std::unique_ptr<vulkan::MeshRenderer> m_mesh_renderer;
    std::unique_ptr<vulkan::ImguiRenderer> m_imgui_renderer;

    vk::Semaphore m_present_semaphore;
    vk::Semaphore m_render_semaphore;
    std::vector<vk::Fence> m_swapchain_image_fences;
    std::vector<vk::CommandBuffer> m_command_buffers;
    std::vector<vk::QueryPool> m_querey_pools;
    ImGuiUtils::ProfilerGraph m_profiler_graph{500};

    bool should_resize_swapchain = false;

  }; // namespace geg
}
        
