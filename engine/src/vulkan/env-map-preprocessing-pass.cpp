#include "env-map-preprocessing-pass.hpp"
#include "imgui.h"

namespace geg::vulkan {
  EnvMapPreprocessPass::EnvMapPreprocessPass(const std::shared_ptr<Device>& device):
      m_device(device) {
    init_pipeline();
  };

  EnvMapPreprocessPass::~EnvMapPreprocessPass() {
    m_device->vkdevice.destroyPipelineLayout(m_pipeline_layout);
    m_device->vkdevice.destroyPipeline(m_pipeline);
  };

  void EnvMapPreprocessPass::render_debug_gui() {
    ImGui::DragFloat("number of samples", &m_settings.number_of_samples, 100.0f, 128, 4098);
    ImGui::DragFloat("mip level", &m_settings.mip_level, 1.0f, 0, 6);
    if (ImGui::Button("recalc importance sampling")) { m_calculated = false; };
    ImGui::Checkbox("recalc importance sampling anyway", &m_calc_anyway);
  }

  void EnvMapPreprocessPass::fill_commands(
      const vk::CommandBuffer& cmd, Scene* scene, const Image& envmap_target) {
    if (!m_calc_anyway) {
      if (m_calculated) return;
      m_calculated = true;
    }

    m_device->transition_image_layout(
        envmap_target.image,
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        cmd,
        6);
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);

    m_settings.width = envmap_target.extent.width;
    m_settings.height = envmap_target.extent.height;
    cmd.pushConstants(
        m_pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(m_settings), &m_settings);

    vk::DescriptorImageInfo img_info{
        .imageView = envmap_target.view,
        .imageLayout = vk::ImageLayout::eGeneral,
    };

    auto output_descriptor =
        m_device->build_descriptor()
            .bind_image(
                0, &img_info, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
            .build()
            .value()
            .first;

    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        m_pipeline_layout,
        0,
        {
            m_tex.descriptor_set,
            output_descriptor,
        },
        nullptr);
    cmd.dispatch(envmap_target.extent.width / 32, envmap_target.extent.height / 18, 1);

    m_device->transition_image_layout(
        envmap_target.image,
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        cmd,
        6);
  };

  void EnvMapPreprocessPass::init_pipeline() {
    auto texture_layout =
        m_device->build_descriptor()
            .bind_image_layout(
                0,
                vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics)
            .build_layout()
            .value();

    auto output_layout =
        m_device->build_descriptor()
            .bind_image_layout(
                0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
            .build_layout()
            .value();

    vk::PushConstantRange push_range{
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(m_settings),
    };

    const std::array<vk::DescriptorSetLayout, 2> layouts = {
        texture_layout,
        output_layout,
    };

    vk::PipelineLayoutCreateInfo layout_info{
        .setLayoutCount = layouts.size(),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    };

    m_pipeline_layout = m_device->vkdevice.createPipelineLayout(layout_info);

    vk::ComputePipelineCreateInfo pipeline_info{
        .stage = m_shader.compute_stage_info,
        .layout = m_pipeline_layout,
    };

    auto res = m_device->vkdevice.createComputePipeline(VK_NULL_HANDLE, pipeline_info);
    GEG_CORE_ASSERT(res.result == vk::Result::eSuccess, "Failed to create compute pipeline!");
    m_pipeline = res.value;
  };
};    // namespace geg::vulkan
