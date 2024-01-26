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
    m_device->vkdevice.destroyPipeline(m_diffuse_pipeline);
    m_device->vkdevice.destroyPipeline(m_specular_pipeline);
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
    auto env_map_tex = asset_manager.get_texture(env_map_cmp.env_map).descriptor_set;

    GEG_CORE_INFO("prefiltering diffuse map");
    Texture* env_map_diffuse = &asset_manager.get_texture(env_map_cmp.env_map_diffuse);
    if (env_map_cmp.env_map_diffuse < 0) {
      env_map_diffuse = new Texture(
          m_device, m_settings.width, m_settings.height, vk::Format::eR32G32B32A32Sfloat);
      env_map_cmp.env_map_diffuse = asset_manager.add_texture(env_map_diffuse);
    }

    env_map_diffuse->transition_layout(vk::ImageLayout::eGeneral);
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_diffuse_pipeline);
    cmd.pushConstants(
        m_pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(m_settings), &m_settings);
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

    GEG_CORE_INFO("prefiltering specular map");
    Texture* env_map_specular = &asset_manager.get_texture(env_map_cmp.env_map_specular);
    if (env_map_cmp.env_map_specular < 0) {
      env_map_specular = new Texture(m_device, 1280, 720, vk::Format::eR32G32B32A32Sfloat, 6);
      env_map_cmp.env_map_specular = asset_manager.add_texture(env_map_specular);
    }

    env_map_specular->transition_layout(vk::ImageLayout::eGeneral);
    m_settings.width = 1280;
    m_settings.height = 720;
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_specular_pipeline);
    cmd.pushConstants(
        m_pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(m_settings), &m_settings);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        m_pipeline_layout,
        0,
        {
            env_map_tex,
            env_map_specular->write_descriptor_set,
        },
        nullptr);
    cmd.dispatch(1280 / 32, 720 / 18, 1);

    GEG_CORE_INFO("integrating brdf");
    Texture* brdf_integration_map = &asset_manager.get_texture(env_map_cmp.brdf_integration);
    if (env_map_cmp.brdf_integration < 0) {
      brdf_integration_map = new Texture(m_device, 512, 512, vk::Format::eR32G32B32A32Sfloat);
      env_map_cmp.brdf_integration = asset_manager.add_texture(brdf_integration_map);
    }
    brdf_integration_map->transition_layout(vk::ImageLayout::eGeneral);
    m_settings.width = 512;
    m_settings.height = 512;
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_brdf_pipeline);
    cmd.pushConstants(
        m_pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(m_settings), &m_settings);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        m_pipeline_layout,
        0,
        {
            env_map_tex,
            brdf_integration_map->write_descriptor_set,
        },
        nullptr);
    cmd.dispatch(512 / 32, 512 / 32, 1);

     env_map_diffuse->transition_layout(vk::ImageLayout::eShaderReadOnlyOptimal);
    // env_map_specular->transition_layout(vk::ImageLayout::eShaderReadOnlyOptimal);
    brdf_integration_map->transition_layout(vk::ImageLayout::eShaderReadOnlyOptimal);
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
            .bind_images_layout(
                0, 6, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
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

    vk::ComputePipelineCreateInfo diffuse_pipeline_info{
        .stage = m_diffuse_shader.compute_stage_info,
        .layout = m_pipeline_layout,
    };

    vk::ComputePipelineCreateInfo specular_pipeline_info{
        .stage = m_specular_shader.compute_stage_info,
        .layout = m_pipeline_layout,
    };

    vk::ComputePipelineCreateInfo brdf_pipeline_info{
        .stage = m_brdf_shader.compute_stage_info,
        .layout = m_pipeline_layout,
    };

    auto res = m_device->vkdevice.createComputePipeline(VK_NULL_HANDLE, diffuse_pipeline_info);
    GEG_CORE_ASSERT(res.result == vk::Result::eSuccess, "Failed to create diffuse pipeline!");
    m_diffuse_pipeline = res.value;

    res = m_device->vkdevice.createComputePipeline(VK_NULL_HANDLE, specular_pipeline_info);
    GEG_CORE_ASSERT(res.result == vk::Result::eSuccess, "Failed to create specular pipeline!");
    m_specular_pipeline = res.value;

    res = m_device->vkdevice.createComputePipeline(VK_NULL_HANDLE, brdf_pipeline_info);
    GEG_CORE_ASSERT(
        res.result == vk::Result::eSuccess, "Failed to create BRDF integration pipeline!");
    m_brdf_pipeline = res.value;
  };
};    // namespace geg::vulkan
