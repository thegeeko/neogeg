#pragma once

#include "assets/asset-manager.hpp"
#include "pch.hpp"

#include "core/layer-stack.hpp"
#include "core/layer.hpp"
#include "core/window.hpp"
#include "events/events.hpp"
#include "vulkan/graphics-context.hpp"
#include "renderer/camera.hpp"
#include "renderer/fps-camera.hpp"
#include "logger.hpp"

namespace geg {
  struct AppInfo {
    std::string name = "Geg App";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool start_maximized = true;
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

    bool is_key_pressed(int keyCode) const { return glfwGetKey(m_window->raw_pointer, keyCode); }
    bool is_button_pressed(int button) const {
      return glfwGetMouseButton(m_window->raw_pointer, button);
    }

    std::pair<double, double> get_mouse_pos() const {
      double x;
      double y;
      glfwGetCursorPos(m_window->raw_pointer, &x, &y);
      return {x, y};
    }

    void attach_layer(Layer *layer) { m_layers.pushLayer(layer); };
    void attach_overlay(Layer *layer) { m_layers.popOverlay(layer); };
    void detach(Layer *layer) {
      layer->on_detach();
      m_layers.popLayer(layer);
    };

    void run();
    static void init_logger();

  private:
    void init();
    bool close(const WindowCloseEvent &e);
    bool pause(const KeyPressedEvent &e) {
      if (e.key_code() == input::KEY_C) paused = !paused;

      return false;
    };
    void event_handler(Event &e);

    bool paused = false;
    bool is_running = true;

    LayerStack m_layers;
    std::shared_ptr<Window> m_window;
    std::unique_ptr<VulkanContext> m_graphics_context;

    // to be refactored
    CameraPositioner_FirstPerson m_camera_controller;
    void update_camera_controls(Event &e);
  };

}    // namespace geg
