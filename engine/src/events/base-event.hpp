#pragma once
#include <string>

namespace geg {

enum class EventType {
  None = 0,
  WindowClose,
  WindowResize,
  WindowFocus,
  WindowLostFocus,
  WindowMoved,
  AppTick,
  AppUpdate,
  AppRender,
  KeyPressed,
  KeyReleased,
  KeyTapped,
  MouseButtonPressed,
  MouseButtonReleased,
  MouseMoved,
  MouseScrolled
};

enum EventCategory {
  None = 0,
  EventCategoryApplication = 1 << 1, // 00001
  EventCategoryInput = 1 << 2,       // 00010
  EventCategoryKeyboard = 1 << 3,    // 00100
  EventCategoryMouse = 1 << 4,       // 01000
  EventCategoryMouseButton = 1 << 5  // 10000
};

class Event {
public:
  Event() = default;

  EventType event_type = EventType::None;
  std::string name = "event";
  EventCategory category_flags = EventCategory::None;
  bool is_handled = false;

  virtual EventType get_event_type() const = 0;
  virtual std::string to_string() const { return name; }

  inline bool from_category(EventCategory category) {
    return category_flags & category;
  }
};

class Dispatcher {
public:
  Dispatcher(Event &_e){};
  template <typename T, typename F>
  static void dispatch(Event &e, const F &cb) {
    if (e.get_event_type() == T::s_EVENT_TYPE) {
      e.is_handled = cb(static_cast<T &>(e));
    }
  }
};
} // namespace geg