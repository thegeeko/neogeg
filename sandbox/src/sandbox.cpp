#include "core/app.hpp"

auto main() -> int {
	geg::App::init_logger();
	auto app = geg::App(geg::AppInfo{});
	app.run();
}
