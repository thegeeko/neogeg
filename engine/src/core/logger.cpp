#include "logger.hpp"

namespace geg {

  std::shared_ptr<spdlog::logger> Logger::core_logger;
  std::shared_ptr<spdlog::logger> Logger::app_logger;
  std::vector<spdlog::sink_ptr> Logger::sinks;

  void Logger::init() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sinks.push_back(console_sink);

    spdlog::set_pattern("%^[%T] %n: %v%$");
    core_logger = std::make_shared<spdlog::logger>("core", begin(sinks), end(sinks));
    core_logger->set_level(spdlog::level::trace);

    app_logger = std::make_shared<spdlog::logger>("core", begin(sinks), end(sinks));
    app_logger->set_level(spdlog::level::trace);
  }

}    // namespace geg
