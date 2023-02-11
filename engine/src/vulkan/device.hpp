#pragma once

#include <memory>

#include "geg-vulkan.hpp"
#include "core/window.hpp"

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

		vk::Device logical_device;
		vk::Instance instance;
		vk::PhysicalDevice physical_device;
		vk::SurfaceKHR surface;

	private:
		bool m_debug_messenger_created = false;
		std::shared_ptr<Window> m_window;
		vk::DebugUtilsMessengerEXT m_debug_messenger;
		std::optional<uint32_t> m_queue_family_index;
		vk::Queue m_graphics_queue;
	};
}		 // namespace geg::vulkan
