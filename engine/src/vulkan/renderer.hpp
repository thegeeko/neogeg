#pragma once

#include "vulkan/device.hpp"
#include "vulkan/swapchain.hpp"
#include "renderer/camera.hpp"

namespace geg::vulkan {
	struct DepthResources {
		vk::Format format = vk::Format::eUndefined;
		vk::ImageView image_view = VK_NULL_HANDLE;
	};

	struct RendererInfo {
		bool should_present = false;
		bool should_clear = false;
	};

	class Renderer {
	public:
		Renderer(
				const std::shared_ptr<Device>& device,
				std::shared_ptr<Swapchain> swapchain,
				std::optional<DepthResources> depth_resources = {},
				RendererInfo info = {});
		virtual ~Renderer();

		void create_framebuffers();
		void cleanup_framebuffers();

		void resize(std::optional<DepthResources> depth_resources = {});

		virtual void fill_commands(
				const vk::CommandBuffer& cmd, const Camera& camera, uint32_t frame_index) = 0;

	protected:
		void begin(const vk::CommandBuffer& cmd, uint32_t image_index);

		std::shared_ptr<Device> m_device;
		std::shared_ptr<Swapchain> m_swapchain;

		vk::RenderPass m_renderpass;
		std::vector<vk::Framebuffer> m_framebuffers;

		bool m_has_depth;
		bool m_should_clear;
		bool m_should_present;

		std::optional<DepthResources> m_depth_resources = {};
	};
};		// namespace geg::vulkan