#pragma once

#include "device.hpp"
#include "glm/vec4.hpp"
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
        uint32_t _mipmap_levels = 1);

    Texture(std::shared_ptr<Device> device, glm::vec<4, uint8_t> color);
    Texture(std::shared_ptr<Device> device, uint32_t width, uint32_t height, vk::Format format, uint32_t _mipmap_levels = 1);
    Texture(Texture&) = delete;
    Texture(Texture&& other) = delete;
    ~Texture();

    vk::DescriptorSet write_descriptor_set;
    vk::DescriptorSet descriptor_set;
    vk::DescriptorSetLayout descriptor_set_layout;
    uint32_t mipmap_levels = 1;
    vk::Image image;
    vk::ImageView image_view;

    std::string name() const { return m_name; }
    void transition_layout(vk::ImageLayout new_layout);

  private:
    void upload_data(const void* img_data);
    void create_texture();
    void generate_mip_levels();

    int32_t m_width = 0;
    int32_t m_height = 0;
    int32_t m_file_channels = 0;
    uint32_t m_channels = 0;
    size_t m_size = 0;

    fs::path m_path;
    std::string m_name;
    vk::Format m_format;
    vk::ImageLayout m_layout = vk::ImageLayout::eUndefined;

    VmaAllocation m_alloc;
    vk::Sampler m_sampler;
    std::vector<vk::ImageView> mips_views;

    std::shared_ptr<Device> m_device;
  };
}    // namespace geg::vulkan
