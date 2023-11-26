#include "meshes.hpp"

#include "assimp/postprocess.h"
#include "assimp/Importer.hpp"
#include "vk_mem_alloc.h"

namespace geg::vulkan {
  Mesh::Mesh(const fs::path& path, const std::shared_ptr<Device>& device) {
    m_device = device;
    m_path = path;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path.string(),
        0 /*aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs*/);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
      GEG_CORE_ASSERT(false, " Error loadin model : ", importer.GetErrorString());
    }

    aiMesh* mesh = scene->mMeshes[0];

    std::vector<Vertex> vertices;
    for (uint32_t i = 0; i != mesh->mNumVertices; i++) {
      const auto v = mesh->mVertices[i];
      const auto n = mesh->mNormals[i];
      const auto t = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i] : aiVector3D{0, 0, 0};

      vertices.push_back(Vertex{
          .position = {v.x, v.y, v.z},
          .normal = {n.x, n.y, n.z},
          .tex_coord = {t.x, t.y},
      });
    }

    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
      aiFace face = mesh->mFaces[i];
      for (uint32_t j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
    }

    const vk::DeviceSize vertex_size = sizeof(Vertex) * vertices.size();
    const vk::DeviceSize index_size = sizeof(indices[1]) * indices.size();

    vertex_offset = 0;
    index_offset = vertex_size;
    size = vertex_size + index_size;

    {
      // final buffer
      auto buffer_info = static_cast<VkBufferCreateInfo>(vk::BufferCreateInfo{
          .size = size,
          .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
          .sharingMode = vk::SharingMode::eExclusive,
      });

      VmaAllocationCreateInfo alloc_info{
          .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      };

      VkBuffer vk_buff;
      vmaCreateBuffer(m_device->allocator, &buffer_info, &alloc_info, &vk_buff, &m_alloc, nullptr);
      buffer = vk_buff;
    }

    auto buffer_info = static_cast<VkBufferCreateInfo>(vk::BufferCreateInfo{
        .size = size,
        .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vk::SharingMode::eExclusive,
    });

    auto memory_flags = static_cast<uint32_t>(
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_CPU_COPY,
        .requiredFlags = memory_flags,
    };

    VkBuffer staging_buffer;
    VmaAllocation staging_alloc;
    vmaCreateBuffer(
        m_device->allocator, &buffer_info, &alloc_info, &staging_buffer, &staging_alloc, nullptr);

    void* mapping_addr = nullptr;
    vmaMapMemory(m_device->allocator, staging_alloc, &mapping_addr);
    void* indecies_mapping_addr = static_cast<uint8_t*>(mapping_addr) + index_offset;

    memcpy(mapping_addr, vertices.data(), vertex_size);
    memcpy(indecies_mapping_addr, indices.data(), index_size);
    vmaUnmapMemory(m_device->allocator, staging_alloc);

    device->single_time_command(
        [&](auto cmd) { device->copy_buffer(staging_buffer, buffer, size, cmd); });

    vmaDestroyBuffer(m_device->allocator, staging_buffer, staging_alloc);

    vk::DescriptorBufferInfo vertx_desc_buff{
        .buffer = buffer,
        .offset = vertex_offset,    // 0
        .range = vertex_size,    // sizeof(Vertex) * vertices.size() 1152
    };

    vk::DescriptorBufferInfo index_desc_buff{
        .buffer = buffer,
        .offset = index_offset,    // 1152
        .range = size - index_offset,    // sizeof(indices[1]) * indices.size() 144
    };

    auto [descriptor, layout] = device->build_descriptor()
                                    .bind_buffer(
                                        0,
                                        &vertx_desc_buff,
                                        vk::DescriptorType::eStorageBuffer,
                                        vk::ShaderStageFlagBits::eVertex)
                                    .bind_buffer(
                                        1,
                                        &index_desc_buff,
                                        vk::DescriptorType::eStorageBuffer,
                                        vk::ShaderStageFlagBits::eVertex)
                                    .build()
                                    .value();

    descriptor_set = descriptor;
    descriptor_set_layout = layout;
  }

  Mesh::~Mesh() {
    vmaDestroyBuffer(m_device->allocator, buffer, m_alloc);
    GEG_CORE_WARN("Destroying mesh");
  }
}    // namespace geg::vulkan
