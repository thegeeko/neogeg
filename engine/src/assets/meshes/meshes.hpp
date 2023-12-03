#pragma once

#include "glm/glm.hpp"
#include "vulkan/geg-vulkan.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/device.hpp"
#include "utils/filesystem.hpp"
#include "assimp/scene.h"

namespace geg::vulkan {

  struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 tex_coord;
    glm::vec2 padding = {0, 0};
  };

  class Mesh {
  public:
    Mesh(const fs::path& path, const std::shared_ptr<Device>& device);
    Mesh(
        const std::shared_ptr<Device>& device,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices);
    ~Mesh();

    vk::DeviceSize size;
    vk::DeviceSize vertex_offset;
    vk::DeviceSize index_offset;

    vk::DescriptorSet descriptor_set;
    vk::DescriptorSetLayout descriptor_set_layout;

    vk::Buffer buffer;
    fs::path path() const { return m_path; };
    std::string name() const { return m_path.filename().string(); }
    uint32_t indices_count() const { return (size - index_offset) / sizeof(uint32_t); };

  private:
    std::shared_ptr<Device> m_device;
    VmaAllocation m_alloc;
    fs::path m_path;

    void upload_to_gpu(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
  };
}    // namespace geg::vulkan
