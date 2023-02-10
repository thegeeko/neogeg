#include "core/app.hpp"
#include "lib.hpp"
#include "core/input.hpp"

auto main() -> int {
  auto app_info = geg::AppInfo {
    "sandbox",
    1280,
    720
  };

  auto app = geg::App(app_info);
  app.run();
}