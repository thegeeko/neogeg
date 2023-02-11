#include "app.hpp"

#include "core/window.hpp"
#include "events/base-event.hpp"
#include "events/events.hpp"
#include "vulkan/renderer.hpp"

namespace geg {
	App::App() {
		Logger::init();

		m_window = std::make_shared<Window>(AppInfo::g_default(), GEG_BIND_CB(event_handler));

		init();
	}

	App::App(const AppInfo &info) {
		Logger::init();

		m_window = std::make_shared<Window>(info, GEG_BIND_CB(event_handler));
		init();
	}

	App::~App() {
		GEG_CORE_WARN("destroying app");
	}

	void App::init() {
		GEG_CORE_INFO("init app");
		m_renderer = std::make_unique<VulkanRenderer>(m_window);
	}

	bool App::deinit(const WindowCloseEvent &e) {
		is_running = false;
		return true;
	}

	void App::event_handler(Event &e) {
		Dispatcher::dispatch<WindowCloseEvent>(e, GEG_BIND_CB(deinit));

		/* GEG_CORE_TRACE(e.to_string()); */
	}

	void App::run() {
		while (is_running) {
			m_window->poll_events();
      m_renderer->render();
		}
	}
}		 // namespace geg
