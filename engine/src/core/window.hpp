#pragma once

#include <functional>
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
    std::pair<uint32_t, uint32_t> dimensions() const {
      int width, height;
      glfwGetWindowSize(raw_pointer, &width, &height);
      return {width, height};
    }

    struct WindowData {
      EventCB events_cb;
    };

  private:
    void register_cb();
    WindowData m_data;
  };

}    // namespace geg
