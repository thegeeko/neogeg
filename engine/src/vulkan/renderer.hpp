#include "renderer/renderer.hpp"
#include "vulkan/device.hpp"
#include "vulkan/swapchain.hpp"

namespace geg {
	class VulkanRenderer: public Renderer {
	public:
		VulkanRenderer(std::shared_ptr<Window> window);
		~VulkanRenderer() override;
		void render() override;

	private:
		void init();
		std::shared_ptr<vulkan::Device> m_device;
		std::shared_ptr<vulkan::Swapchain> m_swapchain;

		std::shared_ptr<Window> m_window;
	};
}		 // namespace geg
