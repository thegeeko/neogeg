#include "uniform-buffer.hpp"
#include "vk_mem_alloc.h"

namespace geg::vulkan {
	UniformBuffer::UniformBuffer(std::shared_ptr<Device> device, size_t size, uint32_t num_of_frames):
			m_device(std::move(device)), m_size(size), m_num_of_frames(num_of_frames) {
		m_padded_size = pad_buffer_size(size);

		auto buff_info = static_cast<VkBufferCreateInfo>(vk::BufferCreateInfo{
				.size = m_padded_size * m_num_of_frames,
				.usage = vk::BufferUsageFlagBits::eUniformBuffer,
		});

		VmaAllocationCreateInfo alloc_info{
				.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
		};

		VkBuffer buff;
		vmaCreateBuffer(m_device->allocator, &buff_info, &alloc_info, &buff, &m_alloc, nullptr);
		m_buff = buff;

		auto desc_buff_info = vk::DescriptorBufferInfo{
				.buffer = m_buff,
				.offset = 0,
				.range = m_padded_size * num_of_frames,
		};

		auto [set, layout] = m_device->build_descriptor()
														 .bind_buffer(
																 0,
																 &desc_buff_info,
																 vk::DescriptorType::eUniformBufferDynamic,
																 vk::ShaderStageFlagBits::eAllGraphics)
														 .build()
														 .value();

		descriptor_set = set;
		descriptor_set_layout = layout;
	}

	UniformBuffer::~UniformBuffer() {
		vmaDestroyBuffer(m_device->allocator, m_buff, m_alloc);
	}
}		 // namespace geg::vulkan
