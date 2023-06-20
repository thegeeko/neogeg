#pragma once

#include "meshes/meshes.hpp"
#include "vulkan/device.hpp"
#include "vulkan/texture.hpp"

namespace geg {
	class AssetManager {
	public:
		AssetManager(std::shared_ptr<vulkan::Device> device): m_device(device){};

		uint32_t enqueue_mesh(fs::path mesh_path) {
			uint32_t id = m_meshs_to_load.size();
			m_meshs_to_load.push_back(mesh_path);
			return id;
		};

		uint32_t enqueue_texture(fs::path texture_path, bool color_data, uint32_t channels) {
			uint32_t id = m_textures_to_load.size();

			m_textures_to_load.push_back({
					.path = texture_path,
					.color = color_data,
					.channels = channels,
			});

			return id;
		};

		void load_textures() {
			GEG_CORE_ASSERT(!m_textures_loaded, "Trying to load meshs more than once");

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

				m_textures.emplace_back(
						m_device, tex_info.path, tex_info.path.filename().string(), format, tex_info.channels);
			}

			m_textures_loaded = true;
		};

		void load_meshs() {
			GEG_CORE_ASSERT(!m_meshs_loaded, "Trying to load meshs more than once");

			for (auto& mesh_path : m_meshs_to_load)
				meshs.emplace_back(mesh_path, m_device);

			m_meshs_loaded = true;
		};

		void load_all() {
			load_meshs();
			load_textures();
		}

	private:
		struct TextureInfo {
			fs::path path;
			bool color = false;
			uint32_t channels;
		};

		bool m_meshs_loaded = false;
		bool m_textures_loaded = false;
		// to do make this big continuos buffer
		std::vector<fs::path> m_meshs_to_load;
		std::vector<vulkan::Mesh> meshs;
		std::vector<TextureInfo> m_textures_to_load;
		std::vector<vulkan::Texture> m_textures;
		std::shared_ptr<vulkan::Device> m_device;
	};
}		 // namespace geg
