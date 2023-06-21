#include "app.hpp"

#include "core/window.hpp"
#include "events/base-event.hpp"
#include "events/events.hpp"
#include "vulkan/renderer.hpp"
#include "core/time.hpp"
#include "core/input.hpp"
#include "imgui.h"

namespace geg {
	App::App() {
		m_window = std::make_shared<Window>(AppInfo{}, GEG_BIND_CB(event_handler));
		init();
	}

	App::App(const AppInfo &info) {
		m_window = std::make_shared<Window>(info, GEG_BIND_CB(event_handler));
		init();
	}

	App::~App() {
		GEG_CORE_WARN("destroying app");
	}

	void App::init() {
		GEG_CORE_INFO("init app");
		m_graphics_context = std::make_unique<VulkanContext>(m_window);
		asset_manager = std::move(AssetManager(m_graphics_context->get_rendering_device()));
		m_camera_controller =
				CameraPositioner_FirstPerson(glm::vec3(0.f), {0.f, 0.f, -1.f}, {0.f, 1.f, 0.f});
	}

	void App::init_logger() {
		Logger::init();
	}

	bool App::close(const WindowCloseEvent &e) {
		is_running = false;
		return true;
	}

	void App::event_handler(Event &e) {
		Dispatcher::dispatch<WindowCloseEvent>(e, GEG_BIND_CB(close));
		Dispatcher::dispatch<KeyPressedEvent>(e, GEG_BIND_CB(pause));
		Dispatcher::dispatch<WindowResizeEvent>(e, GEG_BIND_CB(m_graphics_context->resize));
		Dispatcher::dispatch<KeyPressedEvent>(e, GEG_BIND_CB(m_graphics_context->toggle_ui));

		Dispatcher::dispatch<KeyPressedEvent>(e, [](const KeyPressedEvent &event) {
			if (event.key_code() == input::KEY_T) {
				GEG_CORE_INFO("----------- TIME ------------");
				GEG_CORE_INFO("Time : {}", Timer::now());
				GEG_CORE_INFO("TimeMs : {}", Timer::now_ms());
				GEG_CORE_INFO("EngineTime : {}", Timer::geg_now());
				GEG_CORE_INFO("EngineTimeMs : {}", Timer::geg_now_ms());
				GEG_CORE_INFO("EngineDeltaTime : {}", Timer::delta());
			}
			return false;
		});

		update_camera_controls(e);

		for(auto layer: m_layers)
			layer->on_event(e);

		/* GEG_CORE_TRACE(e.to_string()); */
	}

	void geg::App::update_camera_controls(Event &e) {
		Dispatcher::dispatch<KeyPressedEvent>(e, [&](const KeyPressedEvent &event) {
			auto key = event.key_code();

			if (key == input::KEY_W)
				m_camera_controller.movement.forward = true;
			else if (key == input::KEY_S)
				m_camera_controller.movement.backward = true;
			else if (key == input::KEY_A)
				m_camera_controller.movement.left = true;
			else if (key == input::KEY_D)
				m_camera_controller.movement.right = true;
			else if (key == input::SPACE)
				m_camera_controller.movement.down = true;
			else if (key == input::KEY_LEFT_CONTROL)
				m_camera_controller.movement.up = true;
			else if (key == input::KEY_LEFT_SHIFT)
				m_camera_controller.movement.fast_speed = true;

			return false;
		});

		Dispatcher::dispatch<KeyReleasedEvent>(e, [&](const KeyReleasedEvent &event) {
			auto key = event.key_code();

			if (key == input::KEY_W)
				m_camera_controller.movement.forward = false;
			else if (key == input::KEY_S)
				m_camera_controller.movement.backward = false;
			else if (key == input::KEY_A)
				m_camera_controller.movement.left = false;
			else if (key == input::KEY_D)
				m_camera_controller.movement.right = false;
			else if (key == input::SPACE)
				m_camera_controller.movement.down = false;
			else if (key == input::KEY_LEFT_CONTROL)
				m_camera_controller.movement.up = false;
			else if (key == input::KEY_LEFT_SHIFT)
				m_camera_controller.movement.fast_speed = false;

			return false;
		});
	}

	void App::run() {
		asset_manager.load_all();

		while (is_running) {
			Timer::update();
			auto [mouse_x, mouse_y] = get_mouse_pos();
			const auto is_pressed = is_button_pressed(input::MOUSE_BUTTON_4);

			if (is_pressed)
				glfwSetInputMode(m_window->raw_pointer, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			else
				glfwSetInputMode(m_window->raw_pointer, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse |
									 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

			const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y));
			ImGui::Begin("stats", nullptr, flags);
			ImGui::Text("Frame: %lu", Timer::frame_count());
			ImGui::Text("Fps: %i", Timer::fps());
			ImGui::End();

			m_camera_controller.draw_debug_ui();

			m_window->poll_events();
			if (!paused) {
				m_camera_controller.update(Timer::delta(), {mouse_x, mouse_y}, is_pressed);
				for(auto layer : m_layers){
					m_graphics_context->render(Camera{m_camera_controller}, &layer->scene, &asset_manager);
				}
			}
		}
	}
}		 // namespace geg
