#include "events/events.hpp"
#include "renderer/renderer.hpp"
#include "vulkan/device.hpp"
#include "vulkan/renderpass.hpp"
#include "vulkan/swapchain.hpp"
#include "vk_mem_alloc.hpp"

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
		std::vector<vk::Framebuffer> m_framebuffers;

		vma::Allocator m_allocator;
		std::pair<vma::UniqueImage, vma::UniqueAllocation> m_depth_image;
		vk::UniqueImageView m_depth_image_view;

    std::pair<uint32_t, uint32_t> m_current_dimintaions;
	};
}		 // namespace geg
