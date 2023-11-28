#pragma once
#include "pch.hpp"

#include "vulkan/device.hpp"
#include "ecs/scene.hpp"
#include "renderer/camera.hpp"
#include "vulkan/shader.hpp"
#include "vulkan/uniform-buffer.hpp"

namespace geg::vulkan {
  class DepthPass {
  public:
    DepthPass(const std::shared_ptr<Device>& device);
    ~DepthPass();

    void fill_commands(
        const vk::CommandBuffer& cmd,
        const Camera& camera,
        Scene* scene,
        const Image& depth_target);

    glm::mat4 projection = glm::mat4(1);

  private:
    std::shared_ptr<Device> m_device;
    vk::PipelineLayout m_pipeline_layout;
    vk::Pipeline m_pipeline;

    Shader m_shader{m_device, "assets/shaders/early-depth.glsl", "early depth"};

    struct {
      glm::mat4 proj_view = glm::mat4(1);
    } global_data{};
    UniformBuffer m_global_ubo{m_device, sizeof(global_data), 1};

    void init_pipeline();
  };
}    // namespace geg::vulkan
