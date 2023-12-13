#include "texture.hpp"

#include "stb_image.h"
#include "vk_mem_alloc.h"

namespace geg::vulkan {
  Texture::Texture(
      std::shared_ptr<Device> device,
      fs::path image_path,
      std::string image_name,
      vk::Format format,
      uint32_t _mipmap_levels):
      m_name(std::move(image_name)),
      m_path(std::move(image_path)), m_format(format), m_channels(4), m_device(device),
      mipmap_levels(_mipmap_levels) {
    if (m_format != vk::Format::eR32G32B32A32Sfloat) {
      const uint8_t* image_data =
          stbi_load(m_path.c_str(), &m_width, &m_height, &m_file_channels, (int32_t)m_channels);

      GEG_CORE_ASSERT(image_data, "Failed reading image {} from: {}", m_name, m_path.c_str());

      const vk::Extent3D extent = {
          .width = static_cast<uint32_t>(m_width),
          .height = static_cast<uint32_t>(m_height),
          .depth = 1,
      };

      create_texutre(format, extent, image_data);
      stbi_image_free((void*)image_data);
    } else {
      const float* image_data =
          stbi_loadf(m_path.c_str(), &m_width, &m_height, &m_file_channels, (int32_t)m_channels);
      GEG_CORE_ASSERT(image_data, "Failed reading image {} from: {}", m_name, m_path.c_str());

      const vk::Extent3D extent = {
          .width = static_cast<uint32_t>(m_width),
          .height = static_cast<uint32_t>(m_height),
          .depth = 1,
      };

      create_texutre(format, extent, image_data);
      stbi_image_free((void*)image_data);
    }
  };

  Texture::Texture(std::shared_ptr<Device> device, glm::vec<4, uint8_t> color):
      m_name("dummy"), m_format(vk::Format::eR8G8B8A8Unorm), m_channels(4), m_device(device) {
    m_width = 1;
    m_height = 1;
    const uint8_t* data = &color.r;
    const vk::Extent3D extent = {
        .width = 1,
        .height = 1,
        .depth = 1,
    };
    create_texutre(m_format, extent, data);
  };

  template<typename T>
  void Texture::create_texutre(vk::Format format, vk::Extent3D extent, const T* image_data) {
    m_size = m_width * m_height * m_channels * sizeof(T);
    GEG_CORE_TRACE("copying image with size: {}, name: {}", m_size, m_name);
    const auto image_info = static_cast<VkImageCreateInfo>(vk::ImageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = extent,
        .mipLevels = mipmap_levels,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
                 vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    });

    const VmaAllocationCreateInfo alloc_info = {.usage = VMA_MEMORY_USAGE_GPU_ONLY};

    VkImage vk_img;
    vmaCreateImage(m_device->allocator, &image_info, &alloc_info, &vk_img, &m_alloc, nullptr);
    m_image = vk_img;

    vk::ImageLayout layout_after;
    if (mipmap_levels > 1)
      layout_after = vk::ImageLayout::eTransferDstOptimal;
    else
      layout_after = vk::ImageLayout::eShaderReadOnlyOptimal;

    m_device->upload_to_image(m_image, layout_after, m_format, extent, image_data, m_size);

    if (mipmap_levels > 1) generate_mip_levels();

    const vk::ImageViewCreateInfo view_info{
        .image = m_image,
        .viewType = vk::ImageViewType::e2D,
        .format = m_format,
        .subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = mipmap_levels,
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
        .maxLod = static_cast<float>(mipmap_levels),
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = false,
    };

    m_sampler = m_device->vkdevice.createSampler(sampler_info);

    vk::DescriptorImageInfo desc_info{
        .sampler = m_sampler,
        .imageView = m_image_view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };

    auto [set, layout] =
        m_device->build_descriptor()
            .bind_image(
                0,
                &desc_info,
                vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute)
            .build()
            .value();
    descriptor_set = set;
    descriptor_set_layout = layout;
  }

  void Texture::generate_mip_levels() {
    m_device->single_time_command([&](vk::CommandBuffer cmd) {
      int32_t mip_width = m_width;
      int32_t mip_height = m_height;

      for (uint32_t i = 1; i < mipmap_levels; i++) {
        m_device->transition_image_layout(
            m_image,
            m_format,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            cmd,
            1,
            i - 1);

        m_device->transition_image_layout(
            m_image,
            m_format,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            cmd,
            1,
            i);

        vk::ImageBlit blit{};
        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{mip_width, mip_height, 1};
        blit.srcSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = i - 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{
            mip_width / 2 > 1 ? mip_width / 2 : 1, mip_height / 2 > 1 ? mip_height / 2 : 1, 1};
        blit.dstSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = i,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        cmd.blitImage(
            m_image,
            vk::ImageLayout::eTransferSrcOptimal,
            m_image,
            vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eLinear);

        mip_width /= 2;
        mip_height /= 2;

        if (mip_width < 1) mip_width = 1;
        if (mip_height < 1) mip_width = 1;

        m_device->transition_image_layout(
            m_image,
            m_format,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            cmd,
            1,
            i - 1);
      }

      m_device->transition_image_layout(
          m_image,
          m_format,
          vk::ImageLayout::eTransferDstOptimal,
          vk::ImageLayout::eShaderReadOnlyOptimal,
          cmd,
          1,
          mipmap_levels - 1);
    });
  }

  Texture::~Texture() {
    GEG_CORE_WARN("destroying texture: {}", m_name);
    vmaDestroyImage(m_device->allocator, m_image, m_alloc);
    m_device->vkdevice.destroy(m_image_view);
    m_device->vkdevice.destroy(m_sampler);
  }
}    // namespace geg::vulkan
