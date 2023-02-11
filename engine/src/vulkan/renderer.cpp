#include "vulkan/renderer.hpp"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"
#include "vulkan/device.hpp"

namespace geg {
	VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) {
		m_window = window;

		init();
	}

	void VulkanRenderer::init() {
		GEG_CORE_INFO("init vulkan");

		m_device = std::make_shared<vulkan::Device>(m_window);
		m_swapchain = std::make_shared<vulkan::Swapchain>(m_window, m_device);
	}

	VulkanRenderer::~VulkanRenderer() {
		GEG_CORE_WARN("destroying vulkan");
	}

	void VulkanRenderer::render(){
    m_swapchain->recreate();
	};
}		 // namespace geg
