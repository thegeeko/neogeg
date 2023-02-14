#include "shader.hpp"
#include "utils/filesystem.hpp"

namespace geg::vulkan {
	Shader::Shader(
			std::shared_ptr<Device> device, const fs::path& shader_path, const std::string& shader_name) {
		m_shader_path = shader_path;
		m_shader_name = shader_name;
		m_glsl_shader_src = read_file(m_shader_path);
		m_device = device;

		shaderc::Compiler compiler;
		shaderc::CompileOptions vert_options;
		vert_options.AddMacroDefinition("VERTEX_SHADER");

		shaderc::SpvCompilationResult vert_spv = compiler.CompileGlslToSpv(
				m_glsl_shader_src, shaderc_glsl_vertex_shader, m_shader_name.c_str(), vert_options);

		if (vert_spv.GetCompilationStatus() != shaderc_compilation_status_success) {
			GEG_CORE_ERROR("Failed to compile vertex shader: {0}", vert_spv.GetErrorMessage());
		}

		vert_module = m_device->logical_device.createShaderModule({
				.codeSize = sizeof(uint32_t) * (vert_spv.end() - vert_spv.begin()),
				.pCode = vert_spv.begin(),
		});

		vert_stage_info = vk::PipelineShaderStageCreateInfo{
				.stage = vk::ShaderStageFlagBits::eVertex,
				.module = vert_module,
				.pName = "main",
		};

		shaderc::CompileOptions frag_options;
		frag_options.AddMacroDefinition("FRAGMENT_SHADER");
		shaderc::SpvCompilationResult frag_spv = compiler.CompileGlslToSpv(
				m_glsl_shader_src, shaderc_glsl_fragment_shader, m_shader_name.c_str(), frag_options);

		if (frag_spv.GetCompilationStatus() != shaderc_compilation_status_success) {
			GEG_CORE_ERROR("Failed to compile fragment shader: {0}", frag_spv.GetErrorMessage());
		}

		frag_module = m_device->logical_device.createShaderModule({
				.codeSize = sizeof(uint32_t) * (frag_spv.end() - frag_spv.begin()),
				.pCode = frag_spv.begin(),
		});

		frag_stage_info = vk::PipelineShaderStageCreateInfo{
				.stage = vk::ShaderStageFlagBits::eFragment,
				.module = frag_module,
				.pName = "main",
		};

		GEG_CORE_INFO("Shader {0} compiled successfully", m_shader_name);
	}

	Shader::~Shader() {
		m_device->logical_device.destroyShaderModule(vert_module);
		m_device->logical_device.destroyShaderModule(frag_module);
	}
}		 // namespace geg::vulkan
