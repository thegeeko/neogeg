#include "texture.hpp"

#include <utility>
#include "stb_image.h"

namespace geg::vulkan {
	Texture::Texture(
			std::shared_ptr<Device> device,
			fs::path image_path,
			std::string image_name,
			vk::Format format,
			uint32_t channels):
			m_name(std::move(image_name)),
			m_path(std::move(image_path)), m_format(format), m_channels(channels), m_device(device) {
		stbi_set_flip_vertically_on_load(true);
		const uint8_t* image_data =
				stbi_load(m_path.c_str(), &m_width, &m_height, &m_file_channels, (int32_t)m_channels);
		GEG_CORE_ASSERT(image_data, "Failed reading image {} from: {}", m_name, m_path.c_str());

		const vk::Extent3D extent = {
				.width = static_cast<uint32_t>(m_width),
				.height = static_cast<uint32_t>(m_height),
				.depth = 1,
		};

		m_size = m_width * m_height * m_channels;
		const auto image_info = vk::ImageCreateInfo{
				.imageType = vk::ImageType::e2D,
				.format = format,
				.extent = extent,
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = vk::SampleCountFlagBits::e1,
				.tiling = vk::ImageTiling::eOptimal,
				.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
				.sharingMode = vk::SharingMode::eExclusive,
				.initialLayout = vk::ImageLayout::eUndefined,
		};

		const auto alloc_info = vma::AllocationCreateInfo{.usage = vma::MemoryUsage::eGpuOnly};

		auto [image, alloc] = m_device->allocator.createImageUnique(image_info, alloc_info);
		m_image = std::move(image);
		m_alloc = std::move(alloc);

		m_device->upload_to_image(
				m_image.get(),
				vk::ImageLayout::eShaderReadOnlyOptimal,
				m_format,
				extent,
				image_data,
				m_size);

		stbi_image_free((void*)image_data);

		const vk::ImageViewCreateInfo view_info{
				.image = m_image.get(),
				.viewType = vk::ImageViewType::e2D,
				.format = m_format,
				.subresourceRange{
						.aspectMask = vk::ImageAspectFlagBits::eColor,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1,
				},
		};

		m_image_view = m_device->vkdevice.createImageView(view_info);

		const vk::SamplerCreateInfo sampler_info{
				.magFilter = vk::Filter::eLinear,
				.minFilter = vk::Filter::eLinear,
				.mipmapMode = vk::SamplerMipmapMode::eLinear,
				.addressModeU = vk::SamplerAddressMode::eRepeat,
				.addressModeV = vk::SamplerAddressMode::eRepeat,
				.addressModeW = vk::SamplerAddressMode::eRepeat,
				.mipLodBias = 0.0f,
				.anisotropyEnable = false,
				.maxAnisotropy = 0,
				.compareEnable = false,
				.compareOp = vk::CompareOp::eAlways,
				.minLod = 0.0f,
				.maxLod = 0.0f,
				.borderColor = vk::BorderColor::eIntOpaqueBlack,
				.unnormalizedCoordinates = false,
		};

		m_sampler = m_device->vkdevice.createSampler(sampler_info);

		vk::DescriptorImageInfo desc_info{
				.sampler = m_sampler,
				.imageView = m_image_view,
				.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
		};

		auto [set, layout] = m_device->build_descriptor()
														 .bind_image(
																 0,
																 &desc_info,
																 vk::DescriptorType::eCombinedImageSampler,
																 vk::ShaderStageFlagBits::eAllGraphics)
														 .build()
														 .value();
		descriptor_set = set;
		descriptor_set_layout = layout;
	};

	Texture::~Texture() {
		GEG_CORE_WARN("destroying texture: {}", m_name);
		m_device->vkdevice.destroy(m_image_view);
		m_device->vkdevice.destroy(m_sampler);
	}
}		 // namespace geg::vulkan
