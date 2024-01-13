#include "env-map-preprocessing-pass.hpp"
#include "assets/asset-manager.hpp"
#include "imgui.h"
#include "ecs/components.hpp"

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
    ImGui::InputFloat("Diffuse map width", &m_settings.width);
    ImGui::InputFloat("Diffuse map height", &m_settings.height);
    if (ImGui::Button("recalc importance sampling")) { m_calculated = false; };
    ImGui::Checkbox("recalc importance sampling anyway", &m_calc_anyway);
  }

  void EnvMapPreprocessPass::fill_commands(const vk::CommandBuffer& cmd, Scene* scene) {
    if (!m_calc_anyway) {
      if (m_calculated) return;
      m_calculated = true;
    }

    namespace cmps = components;
    auto env_maps = scene->get_reg().group<cmps::EnvMap>();
    GEG_CORE_ASSERT(!env_maps.empty(), "u need to use env map");
    cmps::EnvMap& env_map_cmp = env_maps.get<cmps::EnvMap>(env_maps[0]);
    auto& asset_manager = AssetManager::get();

    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);

    cmd.pushConstants(
        m_pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(m_settings), &m_settings);

    auto env_map_tex = asset_manager.get_texture(env_map_cmp.env_map).descriptor_set;
    Texture* env_map_diffuse = new Texture(m_device, m_settings.width, m_settings.height, vk::Format::eR32G32B32A32Sfloat);
    env_map_diffuse->transition_layout(vk::ImageLayout::eGeneral);

    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        m_pipeline_layout,
        0,
        {
            env_map_tex,
            env_map_diffuse->write_descriptor_set,
        },
        nullptr);
    cmd.dispatch(m_settings.width / 32, m_settings.height / 18, 1);

    env_map_diffuse->transition_layout(vk::ImageLayout::eShaderReadOnlyOptimal);
    env_map_cmp.env_map_diffuse = asset_manager.add_texture(env_map_diffuse);
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
