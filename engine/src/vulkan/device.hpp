#pragma once

#include <memory>

#include "geg-vulkan.hpp"
#include "core/window.hpp"
#include "vk_mem_alloc.hpp"
#include "vulkan/descriptors.hpp"

namespace geg::vulkan {
	class Device {
	public:
		Device(const std::shared_ptr<Window> &window);
		~Device();
		Device(const Device &) = delete;
		Device &operator=(const Device &) = delete;
		Device(Device &&) = delete;
		Device &operator=(Device &&) = delete;

		vk::Device vkdevice;
		vk::Instance instance;
		vk::PhysicalDevice physical_device;
		vk::SurfaceKHR surface;
		std::optional<uint32_t> queue_family_index;
		vk::Queue graphics_queue;
		vk::CommandPool command_pool;
		vma::Allocator allocator;
		std::shared_ptr<Window> window;

		// helpers
		void single_time_command(const std::function<void(vk::CommandBuffer)> &);
		static void copy_buffer(
				vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, vk::CommandBuffer cmd);
		static void copy_buffer_to_image(
				vk::Buffer src, vk::Image dst, vk::Extent3D image_extent, vk::CommandBuffer cmd);
		static void transition_image_layout(
				vk::Image,
				vk::Format,
				vk::ImageLayout old_layout,
				vk::ImageLayout new_layout,
				vk::CommandBuffer cmd);
		void upload_to_buffer(vk::Buffer buffer, void *data, vk::DeviceSize size);
		void upload_to_image(
				vk::Image image,
				vk::ImageLayout layout_after,
				vk::Format format,
				vk::Extent3D extent,
				const void *data,
				vk::DeviceSize size);
		DescriptorBuilder build_descriptor() {
			return DescriptorBuilder::begin(
					m_descriptor_layout_cache.get(), m_descriptor_allocator.get());
		};

	private:
		bool m_debug_messenger_created = false;
		vk::DebugUtilsMessengerEXT m_debug_messenger;

		std::unique_ptr<DescriptorAllocator> m_descriptor_allocator;
		std::unique_ptr<DescriptorLayoutCache> m_descriptor_layout_cache;
	};
}		 // namespace geg::vulkan
