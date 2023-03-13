#pragma once

#include <glslang/Include/ResourceLimits.h>
#include "geg-vulkan.hpp"
#include "device.hpp"
#include "utils/filesystem.hpp"

namespace geg::vulkan {
	class Shader {
	public:
		Shader(
				std::shared_ptr<Device> device,
				const fs::path& shader_path,
				const std::string& shader_name);
		~Shader();

		vk::ShaderModule vert_module;
		vk::ShaderModule frag_module;
		vk::PipelineShaderStageCreateInfo vert_stage_info;
		vk::PipelineShaderStageCreateInfo frag_stage_info;

	private:
		std::shared_ptr<Device> m_device;
		std::string m_shader_path;
		std::string m_shader_name;
		std::string m_glsl_shader_src;

		std::vector<uint32_t> compile_shader(std::string src, vk::ShaderStageFlags stage);
	};
}		 // namespace geg::vulkan
