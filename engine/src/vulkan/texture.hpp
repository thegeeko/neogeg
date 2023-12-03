#pragma once

#include "device.hpp"
#include "vk_mem_alloc.h"
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

    Texture(Texture&) = delete;
    Texture(Texture&& other) = delete; 

    ~Texture();

    vk::DescriptorSet descriptor_set;
    vk::DescriptorSetLayout descriptor_set_layout;
    std::string name() const { return m_name; }

  private:
    int32_t m_width = 0;
    int32_t m_height = 0;
    int32_t m_file_channels = 0;
    uint32_t m_channels = 0;
    size_t m_size = 0;

    fs::path m_path;
    std::string m_name;
    vk::Format m_format;

    VmaAllocation m_alloc;
    vk::Image m_image;
    vk::ImageView m_image_view;
    vk::Sampler m_sampler;

    std::shared_ptr<Device> m_device;
  };
}    // namespace geg::vulkan
