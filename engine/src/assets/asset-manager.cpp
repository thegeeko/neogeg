#include "asset-manager.hpp"
#include "assets/meshes/meshes.hpp"
#include "core/logger.hpp"
#include "ecs/components.hpp"
#include "ecs/entity.hpp"
#include "tiny_gltf.h"
#include "glm/gtc/quaternion.hpp"

namespace geg {
  AssetManager AssetManager::m_instance{};

  void AssetManager::load_scene(Scene* scene, fs::path path) {
    tinygltf::Model file;
    tinygltf::TinyGLTF loader;
    std::string warn;
    std::string err;

    bool res = loader.LoadASCIIFromFile(&file, &err, &warn, path);
    if (!err.empty()) { GEG_CORE_ERROR("{}", err); }
    if (!warn.empty()) { GEG_CORE_ERROR("{}", warn); }
    GEG_CORE_ASSERT(res, "can't load gltf scene");

    tinygltf::Scene gltf_scene = file.scenes[file.defaultScene];

    components::Transform transform_c;
    for (const int ni : gltf_scene.nodes) {
      tinygltf::Node node = file.nodes[ni];

      if (!node.rotation.empty()) {
        glm::quat rotation(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
        transform_c.rotation = glm::eulerAngles(rotation);
      }

      // if(!node.translation.empty())
      //   transform_c.translation = glm::vec3(node.translation[0], node.translation[1],
      //   node.translation[2]);

      if (node.mesh < 0) continue;
      tinygltf::Mesh mesh = file.meshes[node.mesh];

      uint32_t i = 0;
      for (auto& p : mesh.primitives) {
        GEG_CORE_INFO("Loading mesh {} out of {}", i, mesh.primitives.size());
        i++;
        components::PBR pbr_c;

        tinygltf::Material mat = file.materials[p.material];
        pbr_c.color_factor.r = mat.pbrMetallicRoughness.baseColorFactor[0];
        pbr_c.color_factor.g = mat.pbrMetallicRoughness.baseColorFactor[1];
        pbr_c.color_factor.b = mat.pbrMetallicRoughness.baseColorFactor[2];
        pbr_c.emissive_factor.r = mat.emissiveFactor[0];
        pbr_c.emissive_factor.g = mat.emissiveFactor[1];
        pbr_c.emissive_factor.b = mat.emissiveFactor[2];
        pbr_c.metallic_factor = mat.pbrMetallicRoughness.metallicFactor;
        pbr_c.roughness_factor = mat.pbrMetallicRoughness.roughnessFactor;

        if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
          tinygltf::Texture mt =
              file.textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index];
          tinygltf::Image mr_img = file.images[mt.source];
          path.replace_filename(mr_img.uri);
          pbr_c.metallic_roughness = enqueue_texture(path, false, 4);
        }

        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
          tinygltf::Texture bct = file.textures[mat.pbrMetallicRoughness.baseColorTexture.index];
          tinygltf::Image bc_img = file.images[bct.source];
          path.replace_filename(bc_img.uri);
          pbr_c.albedo = enqueue_texture(path, true, 4);
        }

        if (mat.normalTexture.index >= 0) {
          tinygltf::Texture nmt = file.textures[mat.normalTexture.index];
          tinygltf::Image nm_img = file.images[nmt.source];
          path.replace_filename(nm_img.uri);
          pbr_c.normal_map = enqueue_texture(path, false, 4);
        }

        if (mat.emissiveTexture.index >= 0) {
          tinygltf::Texture emt = file.textures[mat.emissiveTexture.index];
          tinygltf::Image em_img = file.images[emt.source];
          path.replace_filename(em_img.uri);
          pbr_c.emissive_map = enqueue_texture(path, true, 4);
        }

        Entity entt = scene->create_entity(node.name);
        entt.add_component<components::PBR>(pbr_c);
        entt.get_component<components::Transform>() = transform_c;

        std::vector<vulkan::Vertex> verts;
        std::vector<uint32_t> inds;

        const float* pos_buf = nullptr;
        const float* norm_buf = nullptr;
        const float* uv_buf = nullptr;
        const float* tan_buf = nullptr;

        const auto attr = p.attributes;
        if (attr.find("POSITION") != attr.end()) {
          const tinygltf::Accessor& accessor = file.accessors[attr.find("POSITION")->second];
          const tinygltf::BufferView& view = file.bufferViews[accessor.bufferView];
          pos_buf = reinterpret_cast<const float*>(
              &(file.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));

          verts.resize(accessor.count);
        }

        if (attr.find("NORMAL") != attr.end()) {
          const tinygltf::Accessor& accessor = file.accessors[attr.find("NORMAL")->second];
          const tinygltf::BufferView& view = file.bufferViews[accessor.bufferView];
          norm_buf = reinterpret_cast<const float*>(
              &(file.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        }

        if (attr.find("TANGENT") != attr.end()) {
          const tinygltf::Accessor& accessor = file.accessors[attr.find("TANGENT")->second];
          const tinygltf::BufferView& view = file.bufferViews[accessor.bufferView];
          if (accessor.type == TINYGLTF_TYPE_VEC4) { GEG_CORE_TRACE("{}: vec4", i); }
          if (accessor.type == TINYGLTF_TYPE_VEC3) { GEG_CORE_TRACE("{}: vec3", i); }
          tan_buf = reinterpret_cast<const float*>(
              &(file.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        }

        if (attr.find("TEXCOORD_0") != attr.end()) {
          const tinygltf::Accessor& accessor = file.accessors[attr.find("TEXCOORD_0")->second];
          const tinygltf::BufferView& view = file.bufferViews[accessor.bufferView];
          uv_buf = reinterpret_cast<const float*>(
              &(file.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        }

        uint32_t i = 0;
        for (auto& vert : verts) {
          vert.position = glm::vec3{
              pos_buf[i * 3],
              // converting to my cam coords
              pos_buf[(i * 3) + 1],
              pos_buf[(i * 3) + 2],
          };

          vert.normal = !norm_buf ? glm::vec3{0} :
                                    glm::normalize(glm::vec3{
                                        norm_buf[i * 3],
                                        norm_buf[(i * 3) + 1],
                                        norm_buf[(i * 3) + 2],
                                    });

          vert.tangent = !tan_buf ? glm::vec3{0} :
                                    glm::vec3{
                                        tan_buf[i * 4],
                                        tan_buf[(i * 4) + 1],
                                        tan_buf[(i * 4) + 2],
                                    };

          vert.tex_coord = !uv_buf ? glm::vec2{0} :
                                     glm::vec2{
                                         uv_buf[i * 2],
                                         uv_buf[(i * 2) + 1],
                                     };
          i++;
        }

        const tinygltf::Accessor& accessor = file.accessors[p.indices];
        const tinygltf::BufferView& bufferView = file.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = file.buffers[bufferView.buffer];

        auto indexCount = static_cast<uint32_t>(accessor.count);

        // glTF supports different component types of indices
        switch (accessor.componentType) {
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
            const uint32_t* buf = reinterpret_cast<const uint32_t*>(
                &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
            for (size_t index = 0; index < accessor.count; index++) {
              inds.push_back(buf[index]);
            }
            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
            const uint16_t* buf = reinterpret_cast<const uint16_t*>(
                &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
            for (size_t index = 0; index < accessor.count; index++) {
              inds.push_back(buf[index]);
            }
            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
            const uint8_t* buf = reinterpret_cast<const uint8_t*>(
                &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
            for (size_t index = 0; index < accessor.count; index++) {
              inds.push_back(buf[index]);
            }
            break;
          }
          default: GEG_CORE_ASSERT(false, "unsupported index");
        }

        auto mesh = new vulkan::Mesh(m_device, verts, inds);
        m_meshs.push_back(mesh);
        entt.add_component<components::Mesh>(++m_curr_mesh);
        GEG_CORE_INFO("Mesh id: {}", m_curr_mesh);

        load_textures();
      }
    }
  };
}    // namespace geg
