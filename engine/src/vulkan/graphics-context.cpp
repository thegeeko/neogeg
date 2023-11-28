#include "graphics-context.hpp"
#include <vulkan/vulkan_core.h>
#include <memory>

#include "core/logger.hpp"
#include "core/time.hpp"
#include "utility"

#include "ProfilerTask.h"
#include "imgui.h"

#include "vulkan/geg-vulkan.hpp"
#include "vulkan/swapchain.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace geg {
  VulkanContext::VulkanContext(std::shared_ptr<Window> window): m_window(std::move(window)) {
    m_device = std::make_shared<vulkan::Device>(m_window);
    m_swapchain = std::make_shared<vulkan::Swapchain>(m_window, m_device);
    m_current_dimensions = vk::Extent2D{
        .width = m_window->dimensions().first,
        .height = m_window->dimensions().second,
    };

    create_depth_resources();

    m_present_semaphore = m_device->vkdevice.createSemaphore(vk::SemaphoreCreateInfo{});
    m_render_semaphore = m_device->vkdevice.createSemaphore(vk::SemaphoreCreateInfo{});
    const auto images_count = m_swapchain->image_count();
    m_swapchain_image_fences.resize(images_count);

    // create fences
    for (auto& fence : m_swapchain_image_fences) {
      fence = m_device->vkdevice.createFence(vk::FenceCreateInfo{
          .flags = vk::FenceCreateFlagBits::eSignaled,
      });
    }

    // create command buffers
    m_command_buffers = m_device->vkdevice.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
        .commandPool = m_device->command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = images_count,
    });

    m_querey_pools.resize(images_count);
    for (auto& q : m_querey_pools) {
      vk::QueryPoolCreateInfo info{};
      info.queryCount = 6;
      info.queryType = vk::QueryType::eTimestamp;

      q = m_device->vkdevice.createQueryPool(info);
    }

    m_early_depth_pass = std::make_unique<vulkan::DepthPass>(m_device);
    m_mesh_renderer = std::make_unique<vulkan::MeshRenderer>(m_device, m_swapchain->format());
    m_imgui_renderer = std::make_unique<vulkan::ImguiRenderer>(
        m_device, m_swapchain->format(), m_swapchain->image_count());
  }

  VulkanContext::~VulkanContext() {
    m_device->vkdevice.waitIdle();
    vmaDestroyImage(m_device->allocator, m_depth_image.first, m_depth_image.second);
    m_device->vkdevice.destroyImageView(m_depth_image_view);
    m_device->vkdevice.destroySemaphore(m_render_semaphore);
    m_device->vkdevice.destroySemaphore(m_present_semaphore);
    for (const auto& fence : m_swapchain_image_fences)
      m_device->vkdevice.destroyFence(fence);

    for (const auto& q : m_querey_pools)
      m_device->vkdevice.destroyQueryPool(q);
  }

  void VulkanContext::create_depth_resources() {
    if (m_depth_image.first || m_depth_image.second) {
      vmaDestroyImage(m_device->allocator, m_depth_image.first, m_depth_image.second);
      m_device->vkdevice.destroyImageView(m_depth_image_view);
    }

    const auto image_info = static_cast<VkImageCreateInfo>(vk::ImageCreateInfo{
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
    });

    constexpr auto alloc_info = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    VkImage img;
    VmaAllocation alloc;
    vmaCreateImage(m_device->allocator, &image_info, &alloc_info, &img, &alloc, nullptr);
    m_depth_image = {img, alloc};

    m_depth_image_view = m_device->vkdevice.createImageView({
        .image = m_depth_image.first,
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

  void VulkanContext::render(const Camera& camera, Scene* scene) {
    if (m_current_dimensions.width == 0 || m_current_dimensions.height == 0) return;

    draw_debug_ui();

    if (m_swapchain->present_mode() != m_debug_ui_settings.present_mode)
      should_resize_swapchain = true;

    if (should_resize_swapchain) {
      m_device->vkdevice.waitIdle();
      m_swapchain->recreate(m_debug_ui_settings.present_mode);
      create_depth_resources();
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

    auto proj = glm::perspective(
        glm::radians(m_debug_ui_settings.fov),
        (float)m_current_dimensions.width / (float)m_current_dimensions.height,
        0.1f,
        100.f);

    m_mesh_renderer->projection = proj;
    m_early_depth_pass->projection = proj;

    auto cmd = m_command_buffers[m_current_image_index];
    cmd.begin(vk::CommandBufferBeginInfo{});

    cmd.resetQueryPool(m_querey_pools[m_current_image_index], 0, 6);

    vulkan::Image color_target = m_swapchain->images()[m_current_image_index];
    vulkan::Image depth_target =
        vulkan::Image{.view = m_depth_image_view, .extent = m_current_dimensions};

    m_device->transition_image_layout(
        color_target.image,
        m_swapchain->format(),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        cmd);

    if (m_debug_ui_settings.mesh_renderer) {
      cmd.writeTimestamp(
          vk::PipelineStageFlagBits::eTopOfPipe, m_querey_pools[m_current_image_index], 0);
      m_early_depth_pass->fill_commands(cmd, camera, scene, depth_target);
      cmd.writeTimestamp(
          vk::PipelineStageFlagBits::eTopOfPipe, m_querey_pools[m_current_image_index], 1);

      cmd.writeTimestamp(
          vk::PipelineStageFlagBits::eTopOfPipe, m_querey_pools[m_current_image_index], 2);
      m_mesh_renderer->fill_commands(cmd, camera, scene, color_target, depth_target);
      cmd.writeTimestamp(
          vk::PipelineStageFlagBits::eBottomOfPipe, m_querey_pools[m_current_image_index], 3);
    }

    if (m_debug_ui_settings.imgui_renderer) {
      cmd.writeTimestamp(
          vk::PipelineStageFlagBits::eTopOfPipe, m_querey_pools[m_current_image_index], 4);

      m_imgui_renderer->fill_commands(cmd, scene, color_target);

      cmd.writeTimestamp(
          vk::PipelineStageFlagBits::eBottomOfPipe, m_querey_pools[m_current_image_index], 5);
    }

    m_device->transition_image_layout(
        color_target.image,
        m_swapchain->format(),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        cmd);

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

    // TODO: fix this
    // uint64_t timestamps[12];
    // vkGetQueryPoolResults(
    //     m_device->vkdevice,
    //     m_querey_pools[m_current_image_index],
    //     0,
    //     6,
    //     sizeof(timestamps),
    //     timestamps,
    //     2*sizeof(uint64_t),
    //     VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    // for(int i = 0; i < 12; i++)
    //   GEG_CORE_INFO("i{}: {}", i, timestamps[i]);

    // double timestamp_p = m_device->physical_device.getProperties().limits.timestampPeriod;
    // double depth_start = double(timestamp_p * timestamps[0]) / 1000000;
    // double depth_end = double(timestamp_p * timestamps[1]) / 1000000;
    // double depth_pass_delta = (depth_end - depth_start);

    // double mesh_start = double(timestamp_p * timestamps[2]) / 1000000;
    // double mesh_end = double(timestamp_p * timestamps[3]) / 1000000;
    // double mesh_pass_delta = (mesh_end - mesh_start);

    // double imgui_start = double(timestamp_p * timestamps[4]) / 1000000;
    // double imgui_end = double(timestamp_p * timestamps[5]) / 1000000;
    // double imgui_pass_delta = (imgui_end - imgui_start);

    // double time_before = 0;

    // legit::ProfilerTask tasks[3];
    // tasks[0] = {
    //     .startTime = time_before,
    //     .endTime = time_before + depth_pass_delta,
    //     .name = "early depth pass",
    //     .color = legit::Colors::clouds,
    // };
    // time_before += depth_pass_delta;

    // tasks[1] = {
    //     .startTime = time_before,
    //     .endTime = time_before + mesh_pass_delta,
    //     .name = "mesh pass",
    //     .color = legit::Colors::emerald,
    // };
    // time_before += mesh_pass_delta;

    // tasks[2] = {
    //     .startTime = time_before,
    //     .endTime = time_before + imgui_pass_delta,
    //     .name = "imgui pass",
    //     .color = legit::Colors::wisteria,
    // };

    // ImGui::Begin("Gpu profiler", 0, ImGuiWindowFlags_NoScrollbar);
    // ImGui::Text("Frame time: %fms (%u fps)", Timer::delta() * 1000, Timer::fps());
    // m_profiler_graph.LoadFrameData(tasks, 3);
    // test.gpuGraph.LoadFrameData(tasks, 3);
    // m_profiler_graph.RenderTimings(300, 20, 200, Timer::frame_count());
    // m_profiler_graph.maxFrameTime = 500 / 1000.f;
    // ImGui::SliderInt("Profiler zoom", &m_profiler_zoom, 1, 1000);
    // ImGui::End();

    // test.gpuGraph.maxFrameTime = 500.f / 1000;
    // test.Render();

    if (present_res == vk::Result::eSuboptimalKHR)
      should_resize_swapchain = true;
    else if (present_res != vk::Result::eSuccess)
      GEG_CORE_ASSERT(false, "unkonwn error when presenting image");
  }

  void VulkanContext::draw_debug_ui() {
    ImGui::Begin("Vulkan Context");

    if (ImGui::CollapsingHeader("Info: "), ImGuiTreeNodeFlags_DefaultOpen) {
      auto props = m_device->physical_device.getProperties();
      ImGui::Text("Device name: %s", props.deviceName.data());
      ImGui::Text("Device type: %s", vk::to_string(props.deviceType).data());
      ImGui::Text(
          "Current dimensions: %d, %d", m_current_dimensions.width, m_current_dimensions.height);
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
      ImGui::Checkbox("Render ImGui", &m_debug_ui_settings.imgui_renderer);
      ImGui::Checkbox("Render Geometry", &m_debug_ui_settings.mesh_renderer);
    }
    ImGui::End();
  }
}    // namespace geg
