#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

namespace geg::vulkan {
  struct Image {
    vk::Image image;
    vk::ImageView view;
    vk::Extent2D extent;
  };
}    // namespace geg::vulkan
