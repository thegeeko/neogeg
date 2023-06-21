#pragma once

#include "vulkan/renderer.hpp"

namespace geg::vulkan {
	class ImguiRenderer final: public Renderer {
	public:
		ImguiRenderer(const std::shared_ptr<Device>& device, std::shared_ptr<Swapchain> swapchain);

		~ImguiRenderer() override;

		void fill_commands(
				const vk::CommandBuffer& cmd,
				const Camera& camera,
				uint32_t frame_index,
				Scene* scene = nullptr,
				AssetManager* asset_manager = nullptr) override;

	private:
		void create_descriptor_pool();
		vk::DescriptorPool m_descriptor_pool;
	};
}		 // namespace geg::vulkan
