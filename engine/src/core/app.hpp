#pragma once

#include "core/layer-stack.hpp"
#include "core/layer.hpp"
#include "core/window.hpp"
#include "events/events.hpp"
#include "vulkan/renderer.hpp"
#include "renderer/camera.hpp"
#include "renderer/fps-camera.hpp"
#include "pch.hpp"

namespace geg {

	struct AppInfo {
		std::string name;
		uint32_t width;
		uint32_t height;

		static auto g_default() -> AppInfo { return AppInfo{"Geg App", 1280, 720}; }
	};

	class App {
	public:
		App();
		App(const AppInfo &info);

		App(App &&) = delete;
		App(const App &) = delete;
		App &operator=(App &&) = delete;
		App &operator=(const App &) = delete;
		~App();

		bool is_key_pressed(int keyCode) { return glfwGetKey(m_window->raw_pointer, keyCode); }

		bool is_button_pressed(int button) { return glfwGetMouseButton(m_window->raw_pointer, button); }

		std::pair<double, double> get_mouse_pos() {
			double x;
			double y;
			glfwGetCursorPos(m_window->raw_pointer, &x, &y);
			return {x, y};
		}

		void attach_layer(Layer *layer) { m_layers.pushLayer(layer); };
		void attach_overlay(Layer *layer) { m_layers.popOverlay(layer); };
		void detach(Layer *layer) { m_layers.popLayer(layer); };

		void run();

	private:
		void init();
		bool close(const WindowCloseEvent &e);
		void event_handler(Event &e);

		bool is_running = true;
		std::shared_ptr<Window> m_window;
		LayerStack m_layers;
		std::unique_ptr<VulkanRenderer> m_renderer;

		// to be refactored
		CameraPositioner_FirstPerson m_camera_controller;
		void update_camera_controles(Event& e);
	};
}		 // namespace geg
