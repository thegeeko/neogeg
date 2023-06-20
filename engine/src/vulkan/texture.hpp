#include "device.hpp"
#include "utils/filesystem.hpp"

namespace geg::vulkan {
	class Texture {
	public:
		Texture(
				std::shared_ptr<Device> device,
				fs::path image_path,
				std::string image_name,
				vk::Format format,
				uint32_t channels);

			Texture(Texture&& other) {
				descriptor_set = other.descriptor_set;
				descriptor_set_layout = other.descriptor_set_layout;

				m_width = other.m_width;
				m_height = other.m_height;
				m_file_channels = other.m_file_channels;
				m_channels = other.m_channels;
				m_size = other.m_size;
				m_path = std::move(other.m_path);
				m_name = std::move(other.m_name);
				m_format = other.m_format;
				m_alloc = std::move(other.m_alloc);
				m_image = std::move(other.m_image);
				m_image_view = other.m_image_view;
				m_sampler = other.m_sampler;
				m_device = other.m_device;
			};

		~Texture();

		vk::DescriptorSet descriptor_set;
		vk::DescriptorSetLayout descriptor_set_layout;

	private:
		int32_t m_width = 0;
		int32_t m_height = 0;
		int32_t m_file_channels = 0;
		uint32_t m_channels = 0;
		size_t m_size = 0;

		fs::path m_path;
		std::string m_name;
		vk::Format m_format;

		vma::UniqueAllocation m_alloc;
		vma::UniqueImage m_image;
		vk::ImageView m_image_view;
		vk::Sampler m_sampler;

		std::shared_ptr<Device> m_device;
	};
}		 // namespace geg::vulkan
