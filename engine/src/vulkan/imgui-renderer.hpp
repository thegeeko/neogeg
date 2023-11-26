#pragma once

#include "device.hpp"
#include "swapchain.hpp"
#include "renderer/camera.hpp"
#include "ecs/scene.hpp"

namespace geg::vulkan {
	class ImguiRenderer {
	public:
		ImguiRenderer(const std::shared_ptr<Device>& device, vk::Format img_format, uint32_t img_count);
		~ImguiRenderer();

		void fill_commands(
			const vk::CommandBuffer& cmd, Scene* scene, const Image& target);

	private:
		void create_descriptor_pool();
		vk::DescriptorPool m_descriptor_pool;
		std::shared_ptr<Device> m_device;
	};
}		 // namespace geg::vulkan
