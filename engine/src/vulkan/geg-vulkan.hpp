#pragma once

#define VULKAN_HPP_ENABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

namespace geg::vulkan {
	struct Image {
		vk::Image image;
		vk::ImageView view;
	};
}		 // namespace geg::vulkan
