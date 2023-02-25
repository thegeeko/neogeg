#pragma once

#include <memory>

#include "geg-vulkan.hpp"
#include "core/window.hpp"
#include "vk_mem_alloc.hpp"
#include "vulkan/descriptors.hpp"

namespace geg::vulkan {
	class Device {
	public:
		Device(std::shared_ptr<Window> window);
		~Device();
		Device(const Device &) = delete;
		Device &operator=(const Device &) = delete;
		Device(Device &&) = delete;
		Device &operator=(Device &&) = delete;

		vk::Device logical_device;
		vk::Instance instance;
		vk::PhysicalDevice physical_device;
		vk::SurfaceKHR surface;
		std::optional<uint32_t> queue_family_index;
		vk::Queue graphics_queue;
		vk::CommandPool command_pool;
		vma::Allocator allocator;

		// helpers
		void single_time_command(std::function<void(vk::CommandBuffer)>);
		void copy_buffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
		void upload_to_buffer(vk::Buffer buffer, void *data, vk::DeviceSize size);
		DescriptorBuilder build_descriptor() {
			return DescriptorBuilder::begin(m_descriptor_layout_cache.get(), m_descriptor_allocator.get());
		};

	private:
		bool m_debug_messenger_created = false;
		std::shared_ptr<Window> m_window;
		vk::DebugUtilsMessengerEXT m_debug_messenger;

		std::unique_ptr<DescriptorAllocator> m_descriptor_allocator;
		std::unique_ptr<DescriptorLayoutCache> m_descriptor_layout_cache;
	};
}		 // namespace geg::vulkan
