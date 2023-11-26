#pragma once

#include "geg-vulkan.hpp"
#include "device.hpp"
#include "vk_mem_alloc.h"

namespace geg::vulkan {
	class UniformBuffer {
	public:
		UniformBuffer(std::shared_ptr<Device> device, size_t size, uint32_t num_of_frames);
		~UniformBuffer();

		vk::DescriptorSetLayout descriptor_set_layout;
		vk::DescriptorSet descriptor_set;

		uint32_t frame_offset(uint32_t frame_index) const { return frame_index * m_padded_size; }
		void write_at_frame(const void* data, size_t size, uint32_t frame_index) {
			void* void_mapping_addr = nullptr;
			vmaMapMemory(m_device->allocator, m_alloc, &void_mapping_addr);

			auto mapping_addr = (uint8_t*)void_mapping_addr;
			mapping_addr += frame_index * m_padded_size;
			memcpy(mapping_addr, data, size);
			vmaUnmapMemory(m_device->allocator, m_alloc);
		}

	private:
		size_t m_size = 0;
		uint32_t m_num_of_frames = 0;
		size_t m_padded_size = 0;

		vk::Buffer m_buff;
		VmaAllocation m_alloc;

		std::shared_ptr<Device> m_device;

		size_t pad_buffer_size(size_t original_size) {
			const auto limits = m_device->physical_device.getProperties().limits;
			const auto min_alignment = limits.minUniformBufferOffsetAlignment;
			size_t aligned_size = original_size;
			if (min_alignment > 0) {
				aligned_size = (aligned_size + min_alignment - 1) & ~(min_alignment - 1);
			}
			return aligned_size;
		}
	};
}		 // namespace geg::vulkan
