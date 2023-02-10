#include <vulkan/vulkan.hpp>

#include "vulkan/renderer.hpp"
#include "vulkan/device.hpp"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

namespace geg {
VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) {
  m_window = window;

  init();
}

void VulkanRenderer::init() {
  GEG_CORE_INFO("init vulkan");
  vulkan::Device device(m_window);
}

void VulkanRenderer::render(){
  
};
} // namespace geg
