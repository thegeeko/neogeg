#pragma once

#include <unordered_map>
#include "assets/asset-manager.hpp"
#include "ecs/scene.hpp"
#include "shader.hpp"
#include "assets/meshes/meshes.hpp"
#include "uniform-buffer.hpp"
#include "glm/gtx/transform.hpp"
#include "texture.hpp"
#include "renderer/camera.hpp"

namespace geg::vulkan {
  class MeshRenderer {
  public:
    MeshRenderer(const std::shared_ptr<Device>& device, vk::Format img_fomrat);
    ~MeshRenderer();

    void fill_commands(
        const vk::CommandBuffer& cmd,
        const Camera& camera,
        Scene* scene,
        const Image& env_map,
        const Image& color_target,
        const Image& depth_target);

    glm::mat4 projection = glm::mat4(1);

  private:
    std::shared_ptr<Device> m_device;
    void init_pipeline(vk::Format img_format);

    struct Light {
      glm::vec4 pos{0};
      glm::vec4 color{0};
    };

    struct {
      glm::mat4 proj;
      glm::mat4 view;
      glm::mat4 proj_view;
      glm::vec3 cam_pos;
      uint32_t lights_count = 0;
      Light lights[100];
      glm::vec4 skylight_dir;
      glm::vec4 skylight_color;
    } global_data{};

    struct {
      glm::vec4 color_factor;
      glm::vec4 emissive_factor;
      float metallic_factor = 0.0f;
      float roughness_factor = 0.0f;
      float ao = 1.0f;
      float _padding;
    } objec_data{};

    struct {
      glm::mat4 model =
          glm::rotate(glm::scale(glm::mat4(1), {0.03, 0.03, 0.03}), 3.140f, {1.0f, 0.0f, 1.0f});
      glm::mat4 norm =
          glm::rotate(glm::scale(glm::mat4(1), {33.3, 33.3, 33.3}), 3.140f, {1.0f, 0.0f, 1.0f});
    } push_data{};

    UniformBuffer m_global_ubo{m_device, sizeof(global_data), 1};
    std::unordered_map<uint32_t, UniformBuffer*> m_objectubo_cache;

    Texture dummy_tex{m_device, glm::vec<4, uint8_t>{255}};

    vk::Pipeline m_pipeline;
    vk::PipelineLayout m_pipeline_layout;
    Shader m_shader{m_device, "assets/shaders/pbr.glsl", "pbr"};
  };

}    // namespace geg::vulkan
