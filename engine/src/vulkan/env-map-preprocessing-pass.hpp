#pragma once

#include "device.hpp"
#include "shader.hpp"
#include "ecs/scene.hpp"
#include "vulkan/texture.hpp"

namespace geg::vulkan {
  class EnvMapPreprocessPass {
    std::shared_ptr<Device> m_device;

  public:
    EnvMapPreprocessPass(const std::shared_ptr<Device>& device);
    ~EnvMapPreprocessPass();

    void fill_commands(const vk::CommandBuffer& cmd, Scene* scene, const Image& envmap_target);
    void render_debug_gui();

    Texture m_tex{m_device, "assets/envmap.hdr", "env-map", vk::Format::eR32G32B32A32Sfloat, 6};

  private:
    Shader m_shader{m_device, "assets/shaders/preprocess-env-map.glsl", "preprocess-env-map", true};

    bool m_calculated = false;
    bool m_calc_anyway = false;

    struct {
      float number_of_samples = 128;
      float mip_level = 0;
      float width, height;
    } m_settings;

    vk::Pipeline m_pipeline;
    vk::PipelineLayout m_pipeline_layout;

    void init_pipeline();
  };
}    // namespace geg::vulkan
