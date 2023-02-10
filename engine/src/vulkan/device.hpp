#pragma once

#include <memory>

#include "core/window.hpp"
#include "geg-vulkan.hpp"

namespace geg::vulkan {
	class Device {
	public:
		Device(std::shared_ptr<Window> window);
		~Device();
		Device(const Device &) = delete;
		Device &operator=(const Device &) = delete;
		Device(Device &&) = delete;
		Device &operator=(Device &&) = delete;

		void init();
		void destroy();

	private:
		bool m_debug_messenger_created = false;
		std::shared_ptr<Window> m_window;
		vk::Instance m_instance;
		vk::DebugUtilsMessengerEXT m_debug_messenger;
		vk::PhysicalDevice m_physical_device;
		std::optional<uint32_t> m_graphics_queue_family_index;
    vk::Queue m_graphics_queue;
		vk::Device m_device;
	};
}		 // namespace geg::vulkan
