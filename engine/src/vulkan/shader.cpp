#include "shader.hpp"
#include <glslang/Include/glslang_c_shader_types.h>

#include "utils/filesystem.hpp"
#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Include/Common.h"

namespace geg::vulkan {
  static TBuiltInResource default_resources{
      .maxLights = 32,
      .maxClipPlanes = 6,
      .maxTextureUnits = 32,
      .maxTextureCoords = 32,
      .maxVertexAttribs = 64,
      .maxVertexUniformComponents = 4096,
      .maxVaryingFloats = 64,
      .maxVertexTextureImageUnits = 32,
      .maxCombinedTextureImageUnits = 80,
      .maxTextureImageUnits = 32,
      .maxFragmentUniformComponents = 4096,
      .maxDrawBuffers = 32,
      .maxVertexUniformVectors = 128,
      .maxVaryingVectors = 8,
      .maxFragmentUniformVectors = 16,
      .maxVertexOutputVectors = 16,
      .maxFragmentInputVectors = 15,
      .minProgramTexelOffset = -8,
      .maxProgramTexelOffset = 7,
      .maxClipDistances = 8,
      .maxComputeWorkGroupCountX = 65535,
      .maxComputeWorkGroupCountY = 65535,
      .maxComputeWorkGroupCountZ = 65535,
      .maxComputeWorkGroupSizeX = 1024,
      .maxComputeWorkGroupSizeY = 1024,
      .maxComputeWorkGroupSizeZ = 64,
      .maxComputeUniformComponents = 1024,
      .maxComputeTextureImageUnits = 16,
      .maxComputeImageUniforms = 8,
      .maxComputeAtomicCounters = 8,
      .maxComputeAtomicCounterBuffers = 1,
      .maxVaryingComponents = 60,
      .maxVertexOutputComponents = 64,
      .maxGeometryInputComponents = 64,
      .maxGeometryOutputComponents = 128,
      .maxFragmentInputComponents = 128,
      .maxImageUnits = 8,
      .maxCombinedImageUnitsAndFragmentOutputs = 8,
      .maxCombinedShaderOutputResources = 8,
      .maxImageSamples = 0,
      .maxVertexImageUniforms = 0,
      .maxTessControlImageUniforms = 0,
      .maxTessEvaluationImageUniforms = 0,
      .maxGeometryImageUniforms = 0,
      .maxFragmentImageUniforms = 8,
      .maxCombinedImageUniforms = 8,
      .maxGeometryTextureImageUnits = 16,
      .maxGeometryOutputVertices = 256,
      .maxGeometryTotalOutputComponents = 1024,
      .maxGeometryUniformComponents = 1024,
      .maxGeometryVaryingComponents = 64,
      .maxTessControlInputComponents = 128,
      .maxTessControlOutputComponents = 128,
      .maxTessControlTextureImageUnits = 16,
      .maxTessControlUniformComponents = 1024,
      .maxTessControlTotalOutputComponents = 4096,
      .maxTessEvaluationInputComponents = 128,
      .maxTessEvaluationOutputComponents = 128,
      .maxTessEvaluationTextureImageUnits = 16,
      .maxTessEvaluationUniformComponents = 1024,
      .maxTessPatchComponents = 120,
      .maxPatchVertices = 32,
      .maxTessGenLevel = 64,
      .maxViewports = 16,
      .maxVertexAtomicCounters = 0,
      .maxTessControlAtomicCounters = 0,
      .maxTessEvaluationAtomicCounters = 0,
      .maxGeometryAtomicCounters = 0,
      .maxFragmentAtomicCounters = 8,
      .maxCombinedAtomicCounters = 8,
      .maxAtomicCounterBindings = 1,
      .maxVertexAtomicCounterBuffers = 0,
      .maxTessControlAtomicCounterBuffers = 0,
      .maxTessEvaluationAtomicCounterBuffers = 0,
      .maxGeometryAtomicCounterBuffers = 0,
      .maxFragmentAtomicCounterBuffers = 1,
      .maxCombinedAtomicCounterBuffers = 1,
      .maxAtomicCounterBufferSize = 16384,
      .maxTransformFeedbackBuffers = 4,
      .maxTransformFeedbackInterleavedComponents = 64,
      .maxCullDistances = 8,
      .maxCombinedClipAndCullDistances = 8,
      .maxSamples = 4,
      .maxMeshOutputVerticesNV = 256,
      .maxMeshOutputPrimitivesNV = 512,
      .maxMeshWorkGroupSizeX_NV = 32,
      .maxMeshWorkGroupSizeY_NV = 1,
      .maxMeshWorkGroupSizeZ_NV = 1,
      .maxTaskWorkGroupSizeX_NV = 32,
      .maxTaskWorkGroupSizeY_NV = 1,
      .maxTaskWorkGroupSizeZ_NV = 1,
      .maxMeshViewCountNV = 4,
      .limits{
          .nonInductiveForLoops = true,
          .whileLoops = true,
          .doWhileLoops = true,
          .generalUniformIndexing = true,
          .generalAttributeMatrixVectorIndexing = true,
          .generalVaryingIndexing = true,
          .generalSamplerIndexing = true,
          .generalVariableIndexing = true,
          .generalConstantMatrixVectorIndexing = true,
      },
  };

  Shader::Shader(
      std::shared_ptr<Device> device,
      const fs::path& shader_path,
      const std::string& shader_name,
      bool compute) {
    m_shader_path = shader_path.string();
    m_shader_name = shader_name;
    m_glsl_shader_src = read_file(m_shader_path);
    m_device = std::move(device);

    glslang_initialize_process();

    if (!compute) {
      auto vert_spv = compile_shader(m_glsl_shader_src, vk::ShaderStageFlagBits::eVertex);
      vert_module = m_device->vkdevice.createShaderModule({
          .codeSize = sizeof(uint32_t) * (vert_spv.size()),
          .pCode = vert_spv.data(),
      });

      vert_stage_info = vk::PipelineShaderStageCreateInfo{
          .stage = vk::ShaderStageFlagBits::eVertex,
          .module = vert_module,
          .pName = "main",
      };

      auto frag_spv = compile_shader(m_glsl_shader_src, vk::ShaderStageFlagBits::eFragment);
      frag_module = m_device->vkdevice.createShaderModule({
          .codeSize = sizeof(uint32_t) * frag_spv.size(),
          .pCode = frag_spv.data(),
      });

      frag_stage_info = vk::PipelineShaderStageCreateInfo{
          .stage = vk::ShaderStageFlagBits::eFragment,
          .module = frag_module,
          .pName = "main",
      };
    } else {
      auto compute_spv = compile_shader(m_glsl_shader_src, vk::ShaderStageFlagBits::eCompute);
      compute_module = m_device->vkdevice.createShaderModule({
          .codeSize = sizeof(uint32_t) * (compute_spv.size()),
          .pCode = compute_spv.data(),
      });

      compute_stage_info = vk::PipelineShaderStageCreateInfo{
          .stage = vk::ShaderStageFlagBits::eCompute,
          .module = compute_module,
          .pName = "main",
      };
    }

    GEG_CORE_INFO("Shader {0} compiled successfully", m_shader_name);

    glslang_finalize_process();
  }

  Shader::~Shader() {
    m_device->vkdevice.destroyShaderModule(vert_module);
    m_device->vkdevice.destroyShaderModule(frag_module);
  }

  std::vector<uint32_t> Shader::compile_shader(std::string src, vk::ShaderStageFlags stage) {
    glslang_stage_t glslang_stage;

    // 13 is the length of the #version line + \n
    if (stage == vk::ShaderStageFlagBits::eVertex) {
      src.insert(13, "#define VERTEX_SHADER \n");
      glslang_stage = GLSLANG_STAGE_VERTEX;
    } else if (stage == vk::ShaderStageFlagBits::eFragment) {
      src.insert(13, "#define FRAGMENT_SHADER \n");
      glslang_stage = GLSLANG_STAGE_FRAGMENT;
    } else if (stage == vk::ShaderStageFlagBits::eCompute) {
      glslang_stage = GLSLANG_STAGE_COMPUTE;
    } else {
      GEG_CORE_ERROR("Shader stage not supported");
    }

    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = glslang_stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_1,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_3,
        .code = src.c_str(),
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = (const glslang_resource_t*)&default_resources,
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shader, &input)) {
      GEG_CORE_ERROR("GLSL preprocessing failed");
      GEG_CORE_ERROR("{}", glslang_shader_get_info_log(shader));
      GEG_CORE_ERROR("{}", glslang_shader_get_info_debug_log(shader));
    }

    if (!glslang_shader_parse(shader, &input)) {
      GEG_CORE_ERROR("GLSL parsing failed");
      GEG_CORE_ERROR("{}", glslang_shader_get_info_log(shader));
      GEG_CORE_ERROR("{}", glslang_shader_get_info_debug_log(shader));
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
      GEG_CORE_ERROR("GLSL linking failed");
      GEG_CORE_ERROR("{}", glslang_program_get_info_log(program));
      GEG_CORE_ERROR("{}", glslang_program_get_info_debug_log(program));
    }

    glslang_program_SPIRV_generate(program, glslang_stage);

    std::vector<uint32_t> spirv;
    spirv.resize(glslang_program_SPIRV_get_size(program));
    glslang_program_SPIRV_get(program, spirv.data());

    {
      const char* spirv_messages = glslang_program_SPIRV_get_messages(program);

      if (spirv_messages) GEG_CORE_ERROR("{}", spirv_messages);
    }

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return spirv;
  }
}    // namespace geg::vulkan
