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

		vk::Device logical_device;
		vk::Instance instance;
		vk::PhysicalDevice physical_device;
		vk::SurfaceKHR surface;
		std::optional<uint32_t> queue_family_index;
		vk::Queue graphics_queue;
		vk::CommandPool command_pool;

		void single_time_command(std::function<void(vk::CommandBuffer)>);

	private:
		bool m_debug_messenger_created = false;
		std::shared_ptr<Window> m_window;
		vk::DebugUtilsMessengerEXT m_debug_messenger;
	};
}		 // namespace geg::vulkan
