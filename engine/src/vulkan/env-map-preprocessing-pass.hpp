#pragma once

#include "device.hpp"
#include "shader.hpp"
#include "ecs/scene.hpp"
#include "texture.hpp"

namespace geg::vulkan {
  class EnvMapPreprocessPass {
    std::shared_ptr<Device> m_device;

  public:
    EnvMapPreprocessPass(const std::shared_ptr<Device>& device);
    ~EnvMapPreprocessPass();

    void fill_commands(const vk::CommandBuffer& cmd, Scene* scene);
    void render_debug_gui();

  private:
    Shader m_diffuse_shader{m_device, "assets/shaders/prefilter-diffuse.glsl", "prefilter-diffuse", true};
    Shader m_specular_shader{m_device, "assets/shaders/prefilter-specular.glsl", "prefilter-specular", true};
    Shader m_brdf_shader{m_device, "assets/shaders/brdf-integration.glsl", "brdf-integration", true};

    bool m_calculated = false;
    bool m_calc_anyway = false;

    struct {
      float number_of_samples = 4098;
      float mip_level = 0;
      float width = 512;
      float height = 288;
    } m_settings;

    vk::Pipeline m_diffuse_pipeline;
    vk::Pipeline m_specular_pipeline;
    vk::Pipeline m_brdf_pipeline;
    vk::PipelineLayout m_pipeline_layout;

    void init_pipeline();
  };
}    // namespace geg::vulkan
