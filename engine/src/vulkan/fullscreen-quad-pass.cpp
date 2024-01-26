#include "fullscreen-quad-pass.hpp"
#include "ecs/components.hpp"
#include "assets/asset-manager.hpp"

namespace geg::vulkan {
  QuadPass::QuadPass(const std::shared_ptr<Device>& device, vk::Format img_format):
      m_device(device) {
    const vk::SamplerCreateInfo sampler_info{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
    };

    m_sampler = m_device->vkdevice.createSampler(sampler_info);
    init_pipeline(img_format);
  }

  QuadPass::~QuadPass() {
    m_device->vkdevice.destroyPipeline(m_pipeline);
    m_device->vkdevice.destroyPipelineLayout(m_pipeline_layout);
  }

  void QuadPass::fill_commands(
      const vk::CommandBuffer& cmd,
      const Camera& camera,
      Scene* scene,
      const Image& input,
      const Image& target) {
    namespace cmps = components;
    auto env_maps = scene->get_reg().group<cmps::EnvMap>();
    GEG_CORE_ASSERT(!env_maps.empty(), "u need to use env map");
    cmps::EnvMap env_map_cmp = env_maps.get<cmps::EnvMap>(env_maps[0]);
    auto& asset_manager = AssetManager::get();

    vk::RenderingAttachmentInfoKHR color_attachment_info{};
    color_attachment_info.imageView = target.view;
    color_attachment_info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    color_attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
    color_attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
    color_attachment_info.clearValue = vk::ClearValue{};

    vk::RenderingInfoKHR rendering_info{};
    rendering_info.renderArea.extent = target.extent;
    rendering_info.pColorAttachments = &color_attachment_info;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.layerCount = 1;

    cmd.beginRendering(rendering_info);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

    m_push_data.inv_view = glm::inverse(camera.view_matrix());
    m_push_data.inv_proj = glm::inverse(projection);

    cmd.pushConstants(
        m_pipeline_layout,
        vk::ShaderStageFlagBits::eAllGraphics,
        0,
        sizeof(m_push_data),
        &m_push_data);

    cmd.setViewport(
        0,
        vk::Viewport{
            .x = 0,
            .y = 0,
            .width = static_cast<float>(target.extent.width),
            .height = static_cast<float>(target.extent.height),
            .minDepth = 0,
            .maxDepth = 1,
        });

    cmd.setScissor(
        0,
        vk::Rect2D{
            .offset = {},
            .extent = target.extent,
        });

    // vk::DescriptorImageInfo img_info{
    //     .sampler = m_sampler,
    //     .imageView = input.view,
    //     .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    // };

    // auto [set, _] = m_device->build_descriptor()
    //                  .bind_image(
    //                      0,
    //                      &img_info,
    //                      vk::DescriptorType::eCombinedImageSampler,
    //                      vk::ShaderStageFlagBits::eFragment)
    //                  .build()
    //                  .value();

    auto env_map_tex = asset_manager.get_texture(env_map_cmp.env_map).descriptor_set;
    auto env_map_diffuse = asset_manager.get_texture(env_map_cmp.env_map_diffuse).descriptor_set;
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        m_pipeline_layout,
        0,
        {
            env_map_diffuse,
            env_map_tex,
        },
        {});

    cmd.draw(3, 1, 0, 0);
    cmd.endRendering();
  }

  void QuadPass::init_pipeline(vk::Format img_format) {
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
        .cullMode = vk::CullModeFlagBits::eFront,
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
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
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
        .size = sizeof(m_push_data),
    };

    auto texture_layout =
        m_device->build_descriptor()
            .bind_image_layout(
                0,
                vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute)
            .build_layout()
            .value();

    auto skybox_layout =
        m_device->build_descriptor()
            .bind_image_layout(
                0,
                vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute)
            .build_layout()
            .value();

    std::array<vk::DescriptorSetLayout, 2> layouts{texture_layout, skybox_layout};

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
};    // namespace geg::vulkan
