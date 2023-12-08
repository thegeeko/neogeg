#pragma once

#include <memory>
#include "ecs/scene.hpp"
#include "meshes/meshes.hpp"
#include "vulkan/device.hpp"
#include "vulkan/texture.hpp"

namespace geg {
  using MeshId = int32_t;
  using TextureId = int32_t;

  class AssetManager {
  public:
    AssetManager(AssetManager&) = delete;

    static void init(std::shared_ptr<vulkan::Device> device) {
      GEG_CORE_ASSERT(!m_instance.m_inited, "Trying to init asset manager more than once!");
      m_instance.m_inited = true;
      m_instance.m_device = device;
    };

    static AssetManager& get() { return m_instance; };

    void deinit() {
      for (auto mesh : m_meshs)
        delete mesh;

      m_meshs.clear();
      m_meshs_to_load.clear();
      m_curr_mesh = -1;

      for(auto tex: m_textures)
        delete tex;

      m_textures.clear();
      m_textures_to_load.clear();
      m_curr_tex = -1;

      m_device = nullptr;
      m_inited = false;
    }

    MeshId enqueue_mesh(fs::path mesh_path) {
      m_meshs_to_load.push_back(mesh_path);
      return ++m_curr_mesh;
    };

    TextureId enqueue_texture(fs::path texture_path, bool color_data, uint32_t channels) {
      m_textures_to_load.push_back({
          .path = texture_path,
          .color = color_data,
          .channels = channels,
      });

      return ++m_curr_tex;
    };

    void load_scene(Scene* scene, fs::path);

    void load_textures() {
      for (auto& tex_info : m_textures_to_load) {
        vk::Format format;
        if (tex_info.channels == 4) {
          format = tex_info.color ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
        } else if (tex_info.channels == 1) {
          GEG_CORE_ASSERT(!tex_info.color, "Unsupported Texture!");
          format = vk::Format::eR8Unorm;
        } else {
          GEG_CORE_ERROR("Unspported Txture!");
        }

        auto* texture = new vulkan::Texture(
            m_device, tex_info.path, tex_info.path.filename().string(), format, tex_info.channels);
        m_textures.push_back(texture);
      }

      m_textures_to_load.clear();
    };

    void load_meshs() {
      for (auto& mesh_path : m_meshs_to_load) {
        auto* mesh = new vulkan::Mesh(mesh_path, m_device);
        m_meshs.push_back(mesh);
      }

      m_curr_mesh += m_meshs_to_load.size() - 1;
      m_meshs_to_load.clear();
    };

    void load_all() {
      load_meshs();
      load_textures();
    }

    const vulkan::Mesh& get_mesh(MeshId id) { return *m_meshs[id]; }
    const vulkan::Texture& get_texture(TextureId id) { return *m_textures[id]; }
    const std::string get_mesh_name(MeshId id) const {
      if (id < 0) return "No mesh";

      return m_meshs[id]->name();
    }
    const std::string get_texture_name(TextureId id) const {
      if (id < 0) return "No texture";
      return m_textures[id]->name();
    }

  private:
    AssetManager() = default;
    bool m_inited = false;
    static AssetManager m_instance;

    struct TextureInfo {
      fs::path path;
      bool color = false;
      uint32_t channels;
    };

    TextureId m_curr_tex = -1;
    TextureId m_curr_mesh = -1;

    // to do make this big continuos buffer
    std::vector<fs::path> m_meshs_to_load;
    std::vector<vulkan::Mesh*> m_meshs;
    std::vector<TextureInfo> m_textures_to_load;
    std::vector<vulkan::Texture*> m_textures;
    std::shared_ptr<vulkan::Device> m_device;
  };
}    // namespace geg
