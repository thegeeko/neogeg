#pragma once

#include "vulkan/geg-vulkan.hpp"
#include "vulkan/device.hpp"

namespace geg::vulkan {
	struct RenderpassInfo {
		bool has_depth = false;
		bool m_should_clear = false;
		bool should_present = false;
		vk::Format depth_format = vk::Format::eUndefined;
		vk::Format color_format = vk::Format::eUndefined;
	};

	vk::RenderPass create_renderpass(std::shared_ptr<Device> device, const RenderpassInfo info);
}		 // namespace geg::vulkan