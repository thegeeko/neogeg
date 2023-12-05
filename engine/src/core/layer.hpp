#pragma once

#include "assets/asset-manager.hpp"
#include "events/base-event.hpp"
#include "pch.hpp"
#include "ecs/scene.hpp"
#include "renderer/camera.hpp"

namespace geg {

  class Layer {
  public:
    Layer(const std::string &name = "Layer"): m_debug_name(name){};
    virtual ~Layer() = default;

    virtual void on_attach() = 0;
    virtual void on_detach() = 0;
    virtual void update(float ts, CameraPositionerInterface* cam) = 0;
    virtual void ui(float ts) = 0;
    virtual void on_event(Event &event) = 0;
    virtual const std::string &name() const { return m_debug_name; }

    Scene scene;

  protected:
    std::string m_debug_name;
  };
}    // namespace geg
