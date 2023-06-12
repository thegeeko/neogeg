#include "uniform-buffer.hpp"
#include "vk_mem_alloc.hpp"

namespace geg::vulkan {
	UniformBuffer::UniformBuffer(std::shared_ptr<Device> device, size_t size, uint32_t num_of_frames):
			m_device(std::move(device)), m_size(size), m_num_of_frames(num_of_frames) {
		m_padded_size = pad_buffer_size(size);

		const auto buff_info = vk::BufferCreateInfo{
				.size = m_padded_size * m_num_of_frames,
				.usage = vk::BufferUsageFlagBits::eUniformBuffer,
		};

		const auto alloc_info = vma::AllocationCreateInfo{
				.usage = vma::MemoryUsage::eCpuToGpu,
		};

		auto [buff, alloc] = m_device->allocator.createBufferUnique(buff_info, alloc_info);
		m_buff = std::move(buff);
		m_alloc = std::move(alloc);

		auto desc_buff_info = vk::DescriptorBufferInfo{
				.buffer = m_buff.get(),
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

	UniformBuffer::~UniformBuffer() = default;
}		 // namespace geg::vulkan
