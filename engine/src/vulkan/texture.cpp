#include "texture.hpp"
#include <vulkan/vulkan_enums.hpp>

#include "stb_image.h"
#include "vk_mem_alloc.h"

namespace geg::vulkan {

  Texture::Texture(
      std::shared_ptr<Device> device,
      uint32_t width,
      uint32_t height,
      vk::Format format,
      uint32_t _mipmap_levels):
      m_device(device),
      m_name("unnamed"), m_format(format), mipmap_levels(_mipmap_levels), m_channels(4),
      m_width(width), m_height(height) {
    if (m_format != vk::Format::eR32G32B32A32Sfloat) {
      m_size = m_width * m_height * m_channels * sizeof(uint8_t);
    } else {
      m_size = m_width * m_height * m_channels * sizeof(float);
    }

    create_texture();
  }

  Texture::Texture(
      std::shared_ptr<Device> device,
      fs::path image_path,
      std::string image_name,
      vk::Format format,
      uint32_t _mipmap_levels):
      m_name(std::move(image_name)),
      m_path(std::move(image_path)), m_format(format), m_channels(4), m_device(device),
      mipmap_levels(_mipmap_levels) {
    void* img_data;
    if (m_format != vk::Format::eR32G32B32A32Sfloat) {
      img_data =
          stbi_load(m_path.c_str(), &m_width, &m_height, &m_file_channels, (int32_t)m_channels);
      GEG_CORE_ASSERT(img_data, "Failed reading image {} from: {}", m_name, m_path.c_str());
      m_size = m_width * m_height * m_channels * sizeof(uint8_t);
    } else {
      img_data =
          stbi_loadf(m_path.c_str(), &m_width, &m_height, &m_file_channels, (int32_t)m_channels);
      GEG_CORE_ASSERT(img_data, "Failed reading image {} from: {}", m_name, m_path.c_str());
      m_size = m_width * m_height * m_channels * sizeof(float);
    }

    create_texture();
    upload_data(img_data);
    if (mipmap_levels > 1) generate_mip_levels();
    stbi_image_free(img_data);
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

    m_size = m_width * m_height * m_channels * sizeof(uint8_t);
    create_texture();
    upload_data(data);
  };

  void Texture::upload_data(const void* img_data) {
    const vk::Extent3D extent = {
        .width = static_cast<uint32_t>(m_width),
        .height = static_cast<uint32_t>(m_height),
        .depth = 1,
    };

    vk::ImageLayout layout_after;
    if (mipmap_levels > 1)
      layout_after = vk::ImageLayout::eTransferDstOptimal;
    else
      layout_after = vk::ImageLayout::eShaderReadOnlyOptimal;

    m_device->upload_to_image(image, layout_after, m_format, extent, img_data, m_size);
    m_layout = layout_after;
  };

  void Texture::create_texture() {
    const vk::Extent3D extent = {
        .width = static_cast<uint32_t>(m_width),
        .height = static_cast<uint32_t>(m_height),
        .depth = 1,
    };
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eSampled;

    if (m_format == vk::Format::eR32G32B32A32Sfloat) usage |= vk::ImageUsageFlagBits::eStorage;
    const auto image_info = static_cast<VkImageCreateInfo>(vk::ImageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = m_format,
        .extent = extent,
        .mipLevels = mipmap_levels,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    });

    const VmaAllocationCreateInfo alloc_info = {.usage = VMA_MEMORY_USAGE_GPU_ONLY};

    VkImage vk_img;
    vmaCreateImage(m_device->allocator, &image_info, &alloc_info, &vk_img, &m_alloc, nullptr);
    image = vk_img;

    const vk::ImageViewCreateInfo view_info{
        .image = image,
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
    image_view = m_device->vkdevice.createImageView(view_info);

    mips_views.resize(mipmap_levels);
    for (uint32_t i = 0; i < mipmap_levels; i++) {
      const vk::ImageViewCreateInfo view_info{
          .image = image,
          .viewType = vk::ImageViewType::e2D,
          .format = m_format,
          .subresourceRange{
              .aspectMask = vk::ImageAspectFlagBits::eColor,
              .baseMipLevel = i,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
      };
      mips_views[i] = m_device->vkdevice.createImageView(view_info);
    }
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
        .imageView = image_view,
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
            image,
            m_format,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            cmd,
            1,
            i - 1);

        m_device->transition_image_layout(
            image,
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
            image,
            vk::ImageLayout::eTransferSrcOptimal,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eLinear);

        mip_width /= 2;
        mip_height /= 2;

        if (mip_width < 1) mip_width = 1;
        if (mip_height < 1) mip_width = 1;

        m_device->transition_image_layout(
            image,
            m_format,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            cmd,
            1,
            i - 1);
      }

      m_device->transition_image_layout(
          image,
          m_format,
          vk::ImageLayout::eTransferDstOptimal,
          vk::ImageLayout::eShaderReadOnlyOptimal,
          cmd,
          1,
          mipmap_levels - 1);
    });

    m_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
  }

  void Texture::transition_layout(vk::ImageLayout new_layout) {
    std::vector<vk::DescriptorImageInfo> mips_desc_infos(mipmap_levels);

    auto mips_discriptor_builder = m_device->build_descriptor();
    m_device->single_time_command([&](vk::CommandBuffer cmd) {
      for (uint32_t i = 0; i < mipmap_levels; i++) {
        m_device->transition_image_layout(image, m_format, m_layout, new_layout, cmd, 1, i);
        mips_desc_infos[i] = vk::DescriptorImageInfo{
            .sampler = m_sampler,
            .imageView = mips_views[i],
            .imageLayout = new_layout,
        };
      }
    });

    mips_discriptor_builder.bind_images(
        0,
        mips_desc_infos.data(),
        mipmap_levels,
        vk::DescriptorType::eStorageImage,
        vk::ShaderStageFlagBits::eCompute);

    if (new_layout == vk::ImageLayout::eGeneral) {
      write_descriptor_set = mips_discriptor_builder.build().value().first;
    }

    vk::DescriptorImageInfo image_desc_info{
        .sampler = m_sampler,
        .imageView = image_view,
        .imageLayout = new_layout,
    };

    // for the whole image
    auto [set, layout] =
        m_device->build_descriptor()
            .bind_image(
                0,
                &image_desc_info,
                vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute)
            .build()
            .value();
    descriptor_set = set;
    descriptor_set_layout = layout;

    m_layout = new_layout;
  };

  Texture::~Texture() {
    GEG_CORE_WARN("destroying texture: {}", m_name);
    vmaDestroyImage(m_device->allocator, image, m_alloc);
    m_device->vkdevice.destroy(image_view);
    m_device->vkdevice.destroy(m_sampler);
  }
}    // namespace geg::vulkan
