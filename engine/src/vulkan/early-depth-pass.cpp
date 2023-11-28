#include "early-depth-pass.hpp"
#include "ecs/components.hpp"

namespace geg::vulkan {
  DepthPass::DepthPass(const std::shared_ptr<Device>& device): m_device(device) {
    init_pipeline();
  }

  DepthPass::~DepthPass() {
    m_device->vkdevice.destroyPipelineLayout(m_pipeline_layout);
    m_device->vkdevice.destroyPipeline(m_pipeline);
  }

  void DepthPass::fill_commands(
      const vk::CommandBuffer& cmd, const Camera& camera, Scene* scene, const Image& depth_target) {
    namespace cmps = components;
    GEG_CORE_ASSERT(scene, "rendering empty scene");

    vk::RenderingAttachmentInfoKHR depth_attachment_info{};
    depth_attachment_info.imageView = depth_target.view;
    depth_attachment_info.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    depth_attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
    depth_attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
    depth_attachment_info.clearValue = vk::ClearValue{
        .depthStencil = {
            .depth = 1.0f,
            .stencil = 0,
        }};

    vk::RenderingInfoKHR rendering_info{};
    rendering_info.renderArea.extent = depth_target.extent;
    rendering_info.pColorAttachments = nullptr;
    rendering_info.pDepthAttachment = &depth_attachment_info;
    rendering_info.colorAttachmentCount = 0;
    rendering_info.layerCount = 1;

    cmd.beginRendering(rendering_info);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

    cmd.setViewport(
        0,
        vk::Viewport{
            .x = 0,
            .y = 0,
            .width = static_cast<float>(depth_target.extent.width),
            .height = static_cast<float>(depth_target.extent.height),
            .minDepth = 0,
            .maxDepth = 1,
        });

    cmd.setScissor(
        0,
        vk::Rect2D{
            .offset = {},
            .extent = depth_target.extent,
        });

    global_data.proj_view = projection * camera.view_matrix();
    m_global_ubo.write_at_frame(&global_data, sizeof(global_data), 0);

    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        m_pipeline_layout,
        0,
        {m_global_ubo.descriptor_set},
        {m_global_ubo.frame_offset(0)});

    const auto objects = scene->get_reg().group<cmps::PBR>(entt::get<cmps::Transform, cmps::Mesh>);

    auto& asset_manager = AssetManager::get();
    std::array<glm::mat4, 2> push_data;

    for (auto obj : objects) {
      const auto& transform = objects.get<cmps::Transform>(obj);
      const auto& mesh = objects.get<cmps::Mesh>(obj);

      if (!mesh) continue;

      push_data[0] = transform.model_matrix();
      push_data[1] = transform.normal_matrix();

      cmd.pushConstants(
          m_pipeline_layout,
          vk::ShaderStageFlagBits::eAllGraphics,
          0,
          sizeof(push_data),
          &push_data);

      auto& mesh_descriptor = asset_manager.get_mesh(mesh.id).descriptor_set;

      cmd.bindDescriptorSets(
          vk::PipelineBindPoint::eGraphics,
          m_pipeline_layout,
          1,
          {
              mesh_descriptor,
          },
          {});

      uint32_t indcies_count = asset_manager.get_mesh(mesh.id).indices_count();
      cmd.draw(indcies_count, 1, 0, 0);
    }

    cmd.endRendering();
  }

  void DepthPass::init_pipeline() {
    auto vert_shader_stage = m_shader.vert_stage_info;

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
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eClockwise,
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
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = vk::CompareOp::eLess,
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
        .size = 128,
    };

    // @TODO: automate this
    auto gubo_layout = m_global_ubo.descriptor_set_layout;
    auto geometry_layout =
        m_device->build_descriptor()
            .bind_buffer_layout(
                0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
            .bind_buffer_layout(
                1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
            .build_layout()
            .value();

    const std::array<vk::DescriptorSetLayout, 2> layouts = {
        gubo_layout,
        geometry_layout,
    };

    m_pipeline_layout = m_device->vkdevice.createPipelineLayout(vk::PipelineLayoutCreateInfo{
        .setLayoutCount = layouts.size(),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    });

    // dynamic rendering
    vk::PipelineRenderingCreateInfoKHR rendering_info{
        .colorAttachmentCount = 0,
        .pColorAttachmentFormats = nullptr,
        .depthAttachmentFormat = vk::Format::eD32SfloatS8Uint,
    };

    auto result = m_device->vkdevice.createGraphicsPipeline(
        {},
        {
            .pNext = &rendering_info,
            .stageCount = 1,
            .pStages = &vert_shader_stage,
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
