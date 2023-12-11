#include "env-map-preprocessing-pass.hpp"

namespace geg::vulkan {
  EnvMapPreprocessPass::EnvMapPreprocessPass(const std::shared_ptr<Device>& device):
      m_device(device) {
    init_pipeline();
  };

  EnvMapPreprocessPass::~EnvMapPreprocessPass() {
    m_device->vkdevice.destroyPipelineLayout(m_pipeline_layout);
    m_device->vkdevice.destroyPipeline(m_pipeline);
  };

  void EnvMapPreprocessPass::fill_commands(
      const vk::CommandBuffer& cmd, Scene* scene, const Image& envmap_target) {
    m_device->transition_image_layout(
        envmap_target.image,
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        cmd,
        6);
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);

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

    const std::array<vk::DescriptorSetLayout, 2> layouts = {
        texture_layout,
        output_layout,
    };

    vk::PipelineLayoutCreateInfo layout_info{
        .setLayoutCount = layouts.size(),
        .pSetLayouts = layouts.data(),
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
