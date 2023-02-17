#include "events/events.hpp"
#include "renderer/renderer.hpp"
#include "vulkan/device.hpp"
#include "vulkan/renderpass.hpp"
#include "vulkan/swapchain.hpp"
#include "vk_mem_alloc.hpp"

#include "shader.hpp"
#include "vulkan/pipeline.hpp"

namespace geg {
	class VulkanRenderer: public Renderer {
	public:
		VulkanRenderer(std::shared_ptr<Window> window);
		~VulkanRenderer() override;
		void render() override;
		bool resize(WindowResizeEvent dim) override;

	private:
		void init();
		void create_framebuffers_and_depth();
		void cleanup_framebuffers();

		std::shared_ptr<Window> m_window;

		std::shared_ptr<vulkan::Device> m_device;
		std::shared_ptr<vulkan::Swapchain> m_swapchain;
		std::shared_ptr<vulkan::Renderpass> m_renderpass;
		vma::Allocator m_allocator;
		std::vector<vk::Framebuffer> m_swapchain_framebuffers;

		std::pair<vma::UniqueImage, vma::UniqueAllocation> m_depth_image;
		vk::UniqueImageView m_depth_image_view;

		std::pair<uint32_t, uint32_t> m_current_dimintaions;

    std::shared_ptr<vulkan::Shader> tmp_shader;
    std::shared_ptr<vulkan::GraphicsPipeline> tmp_pipeline;

		vk::CommandPool m_command_pool;
		vk::CommandBuffer m_command_buffer;

		uint32_t m_curr_index = -12;
		vk::Semaphore m_present_semaphore;
		vk::Semaphore m_render_semaphore;
		vk::Fence m_render_fence;
	};
}		 // namespace geg
