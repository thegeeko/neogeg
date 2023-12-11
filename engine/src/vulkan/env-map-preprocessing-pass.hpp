#pragma once

#include "device.hpp"
#include "shader.hpp"
#include "ecs/scene.hpp"
#include "vulkan/texture.hpp"

namespace geg::vulkan {
  class EnvMapPreprocessPass {
  public:
    EnvMapPreprocessPass(const std::shared_ptr<Device>& device);
    ~EnvMapPreprocessPass();

    void fill_commands(const vk::CommandBuffer& cmd, Scene* scene, const Image& envmap_target);

  private:
    std::shared_ptr<Device> m_device;

    Shader m_shader{m_device, "assets/shaders/preprocess-env-map.glsl", "preprocess-env-map", true};
    Texture m_tex{m_device, "assets/envmap.hdr", "env-map", vk::Format::eR32G32B32A32Sfloat, 6};

    vk::Pipeline m_pipeline;
    vk::PipelineLayout m_pipeline_layout;

    void init_pipeline();
  };
}    // namespace geg::vulkan
