#include "window.hpp"

#include "GLFW/glfw3.h"
#include "core/app.hpp"
#include "events/events.hpp"

namespace geg {
	Window::Window(const AppInfo &info, const EventCB &event_cb) {
		bool init_status = glfwInit();
		GEG_CORE_ASSERT(init_status, "Failed to initialize GLFW");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		if (info.start_maximized) glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
		raw_pointer = glfwCreateWindow(info.width, info.height, info.name.c_str(), nullptr, nullptr);

		GEG_CORE_INFO("init window");
		m_data.events_cb = event_cb;

		glfwSetWindowUserPointer(raw_pointer, &m_data);
		register_cb();
	}

	Window::~Window() {
		GEG_CORE_WARN("destroying window");
		glfwDestroyWindow(raw_pointer);
		glfwTerminate();
	}

	void Window::poll_events() const {
		glfwPollEvents();
	}

	void Window::register_cb() {
		glfwSetWindowCloseCallback(raw_pointer, [](GLFWwindow *w) {
			auto cb = (*(WindowData *)glfwGetWindowUserPointer(w)).events_cb;

			WindowCloseEvent e{};
			cb(e);
		});

		glfwSetCursorPosCallback(raw_pointer, [](GLFWwindow *w, double x, double y) {
			auto cb = (*(WindowData *)glfwGetWindowUserPointer(w)).events_cb;
			MouseMovedEvent e(x, y);
			cb(e);
		});

		glfwSetWindowSizeCallback(raw_pointer, [](GLFWwindow *window, int width, int height) {
			auto win_data = *(WindowData *)glfwGetWindowUserPointer(window);

			WindowResizeEvent e(width, height);
			win_data.events_cb(e);
		});

		glfwSetMouseButtonCallback(
				raw_pointer, [](GLFWwindow *window, int button, int action, int mods) {
					auto cb = (*(WindowData *)glfwGetWindowUserPointer(window)).events_cb;

					switch (action) {
						case GLFW_PRESS: {
							MouseButtonPressedEvent e(button);
							cb(e);
							break;
						}
						case GLFW_RELEASE: {
							MouseButtonReleasedEvent e(button);
							cb(e);
							break;
						}
					}
				});

		glfwSetScrollCallback(raw_pointer, [](GLFWwindow *window, double xOffset, double yOffset) {
			auto cb = (*(WindowData *)glfwGetWindowUserPointer(window)).events_cb;

			MouseScrolledEvent e((float)xOffset, (float)yOffset);
			cb(e);
		});

		glfwSetCursorPosCallback(raw_pointer, [](GLFWwindow *window, double xPos, double yPos) {
			auto cb = (*(WindowData *)glfwGetWindowUserPointer(window)).events_cb;

			MouseMovedEvent e((float)xPos, (float)yPos);
			cb(e);
		});

		glfwSetKeyCallback(
				raw_pointer, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
					auto cb = (*(WindowData *)glfwGetWindowUserPointer(window)).events_cb;

					switch (action) {
						case GLFW_PRESS: {
							KeyPressedEvent e(key, 0);
							cb(e);
							break;
						}
						case GLFW_RELEASE: {
							KeyReleasedEvent e(key);
							cb(e);
							break;
						}
						case GLFW_REPEAT: {
							KeyPressedEvent e(key, 1);
							cb(e);
							break;
						}
					}
				});

		glfwSetCharCallback(raw_pointer, [](GLFWwindow *window, uint32_t keycode) {
			auto cb = (*(WindowData *)glfwGetWindowUserPointer(window)).events_cb;
			KeyTappedEvent e(keycode);
			cb(e);
		});
	}
}		 // namespace geg
