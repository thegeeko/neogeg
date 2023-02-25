#pragma once

#include "base-event.hpp"

#define IMPL_EVENT_TYPE(ev)                                                    \
  const static EventType s_EVENT_TYPE = ev;                                    \
  EventType event_type = ev;                                                   \
  [[nodiscard]] EventType get_event_type() const override { return event_type; }

namespace geg {

// ---------------------- window & app events
// ---------------------------------------------------- //
class WindowCloseEvent : public Event {
public:
  WindowCloseEvent() {}

  IMPL_EVENT_TYPE(EventType::WindowClose)

  std::string name = "Window close event";
  char category_flags = EventCategoryApplication;

  std::string to_string() const override { return name; }
};

class WindowResizeEvent : public Event {
public:
  WindowResizeEvent(int _w, int _h) : w(_w), h(_h) {}

  IMPL_EVENT_TYPE(EventType::WindowResize)

  std::string name = "Window resized";
  char category_flags = EventCategoryApplication;

  inline uint32_t width() const { return w; }

  inline uint32_t height() const { return h; }

  std::string to_string() const override {
    std::string str;
    str = "Resized the window to : ";
    str += "w = " + std::to_string(w);
    str += ", h = " + std::to_string(h);
    return str;
  }

private:
  const int w, h;
};

class AppTickEvent : public Event {
public:
  IMPL_EVENT_TYPE(EventType::AppTick)
  std::string name = "app tick";
  char category_flags = EventCategoryApplication;
  std::string to_string() const override { return name; }
};

class AppUpdateEvent : public Event {
public:
  IMPL_EVENT_TYPE(EventType::AppUpdate)
  std::string name = "app update";
  char category_flags = EventCategoryApplication;
  std::string to_string() const override { return name; }
};

class AppRenderEvent : public Event {
public:
  IMPL_EVENT_TYPE(EventType::AppRender)
  std::string name = "app rendered";
  char category_flags = EventCategoryApplication;
  std::string to_string() const override { return name; }
};

// ---------------------- window & app events
// ---------------------------------------------------- //

// ---------------------- keyboard events
// ---------------------------------------------------- //

class KeyPressedEvent : public Event {
public:
  IMPL_EVENT_TYPE(EventType::KeyPressed);

  KeyPressedEvent(int keycode, int repeat_count)
      : m_keycode(keycode), m_repeat_count(repeat_count) {}
  inline int repeat_count() const { return m_repeat_count; }
  char category_flags =
      EventCategory::EventCategoryInput | EventCategory::EventCategoryKeyboard;
  int key_code() const { return m_keycode; }

  std::string to_string() const override {
    std::string str = "pressed key " + std::to_string(m_keycode);
    str += " " + std::to_string(m_repeat_count) + "times";
    return str;
  }

private:
  int m_repeat_count;
  int m_keycode;
};

class KeyReleasedEvent : public Event {
public:
  IMPL_EVENT_TYPE(EventType::KeyReleased);

  KeyReleasedEvent(int keycode) : m_keycode(keycode) {}
  char category_flags =
      EventCategory::EventCategoryInput | EventCategory::EventCategoryKeyboard;
  int key_code() const { return m_keycode; }

  std::string to_string() const override {
    std::string str = "released key " + std::to_string(m_keycode);
    return str;
  }

private:
  int m_keycode;
};

class KeyTappedEvent : public Event {
public:
  KeyTappedEvent(int keycode) : m_keycode(keycode) {}

  IMPL_EVENT_TYPE(EventType::KeyTapped);
  char category_flags =
      EventCategory::EventCategoryInput | EventCategory::EventCategoryKeyboard;
  int get_key_code() { return m_keycode; }

  std::string to_string() const override {
    std::string str = "Tapped key " + std::to_string(m_keycode);
    return str;
  }

private:
  int m_keycode;
};

// ---------------------- keyboard events
// ---------------------------------------------------- //

// ---------------------- mouse events
// ---------------------------------------------------- //

class MouseMovedEvent : public Event {
public:
  MouseMovedEvent(float _x, float _y) : x(_x), y(_y) {}

  IMPL_EVENT_TYPE(EventType::MouseMoved);
  std::string name = "Mouse Moved event";
  char category_flags =
      EventCategory::EventCategoryInput | EventCategory::EventCategoryMouse;
  inline float get_x() const { return x; }
  inline float get_y() const { return y; }

  std::string to_string() const override {
    std::string str;
    str = "Mouse Moved : ";
    str += "x = " + std::to_string(x);
    str += ", y = " + std::to_string(y);
    return str;
  }

private:
  const float x, y;
};

class MouseScrolledEvent : public Event {
public:
  MouseScrolledEvent(float x_offset, float y_offset)
      : m_x_offset(x_offset), m_y_offset(y_offset) {}

  IMPL_EVENT_TYPE(EventType::MouseScrolled);
  inline float x_offset() const { return m_x_offset; }
  inline float y_offset() const { return m_y_offset; }
  char category_flags = EventCategoryMouse | EventCategoryInput;

  std::string to_string() const override {
    std::string str;
    str = "Mouse Scrolled : ";
    str += "x = " + std::to_string(m_x_offset);
    str += ", y = " + std::to_string(m_y_offset);
    return str;
  }

private:
  float m_x_offset, m_y_offset;
};

class MouseButtonPressedEvent : public Event {
public:
  MouseButtonPressedEvent(int button) : m_button(button) {}

  IMPL_EVENT_TYPE(EventType::MouseButtonPressed);
  inline int getMouseButton() const { return m_button; }
  char category_flags =
      EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton;

  std::string to_string() const override {
    std::string str = "pressed : " + std::to_string(m_button);
    return str;
  }

protected:
  int m_button;
};

class MouseButtonReleasedEvent : public Event {
public:
  MouseButtonReleasedEvent(int button) : m_button(button) {}

  IMPL_EVENT_TYPE(EventType::MouseButtonReleased);
  inline int getMouseButton() const { return m_button; }
  char category_flags =
      EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton;

  std::string to_string() const override {
    std::string str = "released : " + std::to_string(m_button);
    return str;
  }

protected:
  int m_button;
};

// ---------------------- mouse events
// ---------------------------------------------------- //

} // namespace geg