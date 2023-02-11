#pragma once

#include "GLFW/glfw3.h"
#include "events/base-event.hpp"

namespace geg {
	using EventCB = std::function<void(Event &)>;
	struct AppInfo;

	class Window {
	public:
		Window(const AppInfo &info, const EventCB &event_cb);
		Window(const Window &) = delete;
		Window(Window &&) = delete;
		Window &operator=(const Window &) = delete;
		Window &operator=(Window &&) = delete;
		~Window();

		void poll_events() const;
		GLFWwindow *raw_pointer;
		uint32_t width() const { return m_data.width; }
		uint32_t height() const { return m_data.height; }

		struct WindowData {
			uint32_t width;
			uint32_t height;
			EventCB events_cb;
		};

	private:
		void register_cb();
		WindowData m_data;
	};

}		 // namespace geg
