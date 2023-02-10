#pragma once

namespace geg::vulkan {
#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      GEG_CORE_ERROR("Vulkan error: {}", err);                                 \
      exit(-1);                                                                \
    }                                                                          \
  } while (0)
} // namespace geg::vulkan
