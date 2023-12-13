#pragma once

#include "device.hpp"
#include "shader.hpp"
#include "renderer/camera.hpp"
#include "texture.hpp"

namespace geg::vulkan {

  class QuadPass {
  public:
    QuadPass(const std::shared_ptr<Device>& device, vk::Format img_fomrat);
    ~QuadPass();

    void fill_commands(
        const vk::CommandBuffer& cmd,
        const Camera& camera,
        const Texture& sky_map,
        const Image& input,
        const Image& target);

    glm::mat4 projection;

  private:
    void init_pipeline(vk::Format img_format);

    struct {
      glm::mat4 inv_view;
      glm::mat4 inv_proj;
    } m_push_data;
    const std::shared_ptr<Device> m_device;
    vk::Pipeline m_pipeline;
    vk::Sampler m_sampler;
    vk::PipelineLayout m_pipeline_layout;
    Shader m_shader{m_device, "assets/shaders/fullscreen-quad.glsl", "fullscreen-quad"};
  };
}    // namespace geg::vulkan
