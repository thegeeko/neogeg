#include "mesh-renderer.hpp"

#include "assets/asset-manager.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include "ecs/components.hpp"
#include "vulkan/uniform-buffer.hpp"

namespace geg::vulkan {

  MeshRenderer::MeshRenderer(const std::shared_ptr<Device>& device, vk::Format img_format):
      m_device(device) {
    init_pipeline(img_format);
  }

  MeshRenderer::~MeshRenderer() {
    m_device->vkdevice.destroyPipeline(m_pipeline);
    m_device->vkdevice.destroyPipelineLayout(m_pipeline_layout);
  }

  void MeshRenderer::fill_commands(
      const vk::CommandBuffer& cmd,
      const Camera& camera,
      Scene* scene,
      const Image& color_target,
      const Image& depth_target) {
    namespace cmps = components;
    if (!scene) return;
    auto& asset_manager = AssetManager::get();

    vk::RenderingAttachmentInfoKHR color_attachment_info{};
    color_attachment_info.imageView = color_target.view;
    color_attachment_info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    color_attachment_info.loadOp = vk::AttachmentLoadOp::eLoad;
    color_attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
    color_attachment_info.clearValue = vk::ClearValue{};

    vk::RenderingAttachmentInfoKHR depth_attachment_info{};
    depth_attachment_info.imageView = depth_target.view;
    depth_attachment_info.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    depth_attachment_info.loadOp = vk::AttachmentLoadOp::eLoad;
    depth_attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
    depth_attachment_info.clearValue = vk::ClearValue{};

    vk::RenderingInfoKHR rendering_info{};
    rendering_info.renderArea.extent = color_target.extent;
    rendering_info.pColorAttachments = &color_attachment_info;
    rendering_info.pDepthAttachment = &depth_attachment_info;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.layerCount = 1;

    cmd.beginRendering(rendering_info);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

    cmd.setViewport(
        0,
        vk::Viewport{
            .x = 0,
            .y = 0,
            .width = static_cast<float>(color_target.extent.width),
            .height = static_cast<float>(color_target.extent.height),
            .minDepth = 0,
            .maxDepth = 1,
        });

    cmd.setScissor(
        0,
        vk::Rect2D{
            .offset = {},
            .extent = color_target.extent,
        });

    global_data.proj = projection;
    global_data.view = camera.view_matrix();
    global_data.proj_view = projection * camera.view_matrix();
    global_data.cam_pos = camera.position();
    const auto& light_view = scene->get_reg().view<cmps::Light, cmps::Transform>();
    uint32_t i = 0;
    for (auto& light : light_view) {
      auto& light_color = light_view.get<cmps::Light>(light).light_color;
      auto& pos = light_view.get<cmps::Transform>(light).translation;

      global_data.lights[i] = {
          .pos = {pos, 1.0f},
          .color = light_color,
      };

      i++;
    }
    global_data.lights_count = i;
    auto sky_lights = scene->get_reg().group<cmps::SkyLight>();
    if (!sky_lights.empty()) {
      global_data.skylight_dir = {sky_lights.get<cmps::SkyLight>(sky_lights[0]).direction, 1.0f};
      global_data.skylight_color = sky_lights.get<cmps::SkyLight>(sky_lights[0]).color;
    }

    // update the ubos
    m_global_ubo.write_at_frame(&global_data, sizeof(global_data), 0);

    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        m_pipeline_layout,
        0,
        {m_global_ubo.descriptor_set},
        {m_global_ubo.frame_offset(0)});

    auto env_maps = scene->get_reg().group<cmps::EnvMap>();
    GEG_CORE_ASSERT(!env_maps.empty(), "u need to use env map");
    cmps::EnvMap& env_map_cmp = env_maps.get<cmps::EnvMap>(env_maps[0]);

    const auto objects = scene->get_reg().group<cmps::PBR>(entt::get<cmps::Transform, cmps::Mesh>);
    for (auto obj : objects) {
      const auto& pbr_data = objects.get<cmps::PBR>(obj);
      const auto& transform = objects.get<cmps::Transform>(obj);
      const auto& mesh = objects.get<cmps::Mesh>(obj);
      const auto obj_id = static_cast<uint32_t>(obj);

      if (!mesh) {
        GEG_CORE_WARN("no mesh data in some mesh");
        continue;
      }

      push_data.model = transform.model_matrix();
      push_data.norm = transform.normal_matrix();

      cmd.pushConstants(
          m_pipeline_layout,
          vk::ShaderStageFlagBits::eAllGraphics,
          0,
          sizeof(push_data),
          &push_data);

      objec_data.color_factor = glm::vec4(pbr_data.color_factor, 1.0f);
      objec_data.emissive_factor = glm::vec4(pbr_data.emissive_factor, 1.0f);
      objec_data.metallic_factor = pbr_data.metallic_factor;
      objec_data.roughness_factor = pbr_data.roughness_factor;
      objec_data.ao = pbr_data.AO;

      if (m_objectubo_cache.find(obj_id) == m_objectubo_cache.end()) {
        m_objectubo_cache[obj_id] = new UniformBuffer(m_device, sizeof(objec_data), 1);
      }
      m_objectubo_cache[obj_id]->write_at_frame(&objec_data, sizeof(objec_data), 0);

      auto& mesh_descriptor = asset_manager.get_mesh(mesh.id).descriptor_set;
      auto& albedo_descriptor = (pbr_data.albedo >= 0) ?
                                    asset_manager.get_texture(pbr_data.albedo).descriptor_set :
                                    dummy_tex.descriptor_set;

      auto& metallic_roughness_descriptor =
          (pbr_data.metallic_roughness >= 0) ?
              asset_manager.get_texture(pbr_data.metallic_roughness).descriptor_set :
              dummy_tex.descriptor_set;

      auto& normal_descriptor = (pbr_data.normal_map >= 0) ?
                                    asset_manager.get_texture(pbr_data.normal_map).descriptor_set :
                                    dummy_tex.descriptor_set;

      auto& emissive_descriptor =
          (pbr_data.emissive_map >= 0) ?
              asset_manager.get_texture(pbr_data.emissive_map).descriptor_set :
              dummy_tex.descriptor_set;

      auto env_diffuse_descriptor = asset_manager.get_texture(env_map_cmp.env_map_diffuse).descriptor_set;
      auto env_specular_descriptor = asset_manager.get_texture(env_map_cmp.env_map_specular).descriptor_set;
      auto env_brdflut_descriptor = asset_manager.get_texture(env_map_cmp.brdf_integration).descriptor_set;

      cmd.bindDescriptorSets(
          vk::PipelineBindPoint::eGraphics,
          m_pipeline_layout,
          1,
          {
              m_objectubo_cache[obj_id]->descriptor_set,
              mesh_descriptor,
              albedo_descriptor,
              metallic_roughness_descriptor,
              normal_descriptor,
              emissive_descriptor,
              env_diffuse_descriptor,
              env_specular_descriptor,
              env_brdflut_descriptor,
          },
          {
              m_objectubo_cache[obj_id]->frame_offset(0),
          });

      uint32_t indcies_count = asset_manager.get_mesh(mesh.id).indices_count();
      cmd.draw(indcies_count, 1, 0, 0);
    }

    cmd.endRendering();
  }

  void MeshRenderer::init_pipeline(vk::Format img_format) {
    auto vert_shader_stage = m_shader.vert_stage_info;
    auto frag_shader_stage = m_shader.frag_stage_info;

    auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo{};

    auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = VK_FALSE,
    };

    vk::PipelineViewportStateCreateInfo viewport_state{
        .viewportCount = 1,
        .scissorCount = 1,
    };

    std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    auto rasterizer = vk::PipelineRasterizationStateCreateInfo{
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    auto multisampling = vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
    };

    auto depth_stencil = vk::PipelineDepthStencilStateCreateInfo{
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = vk::CompareOp::eEqual,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    auto color_blend_attachment = vk::PipelineColorBlendAttachmentState{
        .blendEnable = VK_FALSE,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    auto color_blending = vk::PipelineColorBlendStateCreateInfo{
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    vk::PushConstantRange push_range{
        .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
        .offset = 0,
        .size = sizeof(push_data),
    };

    // @TODO: automate this
    auto gubo_layout = m_global_ubo.descriptor_set_layout;
    auto oubo_layout = m_global_ubo.descriptor_set_layout;
    auto geometry_layout =
        m_device->build_descriptor()
            .bind_buffer_layout(
                0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
            .bind_buffer_layout(
                1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
            .build_layout()
            .value();

    auto texture_layout =
        m_device->build_descriptor()
            .bind_image_layout(
                0,
                vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute)
            .build_layout()
            .value();

    const std::array<vk::DescriptorSetLayout, 10> layouts = {
        gubo_layout,
        oubo_layout,
        geometry_layout,
        texture_layout,
        texture_layout,
        texture_layout,
        texture_layout,
        texture_layout,
        texture_layout,
        texture_layout,
    };

    m_pipeline_layout = m_device->vkdevice.createPipelineLayout(vk::PipelineLayoutCreateInfo{
        .setLayoutCount = layouts.size(),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    });

    // dynamic rendering
    vk::PipelineRenderingCreateInfoKHR rendering_info{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &img_format,
        .depthAttachmentFormat = vk::Format::eD32SfloatS8Uint,
    };

    auto result = m_device->vkdevice.createGraphicsPipeline(
        VK_NULL_HANDLE,
        {
            .pNext = &rendering_info,
            .stageCount = 2,
            .pStages = std::array{vert_shader_stage, frag_shader_stage}.data(),
            .pVertexInputState = &vertex_input_info,
            .pInputAssemblyState = &input_assembly,
            .pViewportState = &viewport_state,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depth_stencil,
            .pColorBlendState = &color_blending,
            .pDynamicState = &dynamic_state,
            .layout = m_pipeline_layout,
            .renderPass = nullptr,
            .subpass = 0,
            .basePipelineHandle = vk::Pipeline{},
            .basePipelineIndex = -1,
        });

    GEG_CORE_ASSERT(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline!");

    m_pipeline = result.value;
  }

}    // namespace geg::vulkan
