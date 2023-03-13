#pragma once

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <memory>

namespace geg {
	class Logger {
	public:
		static void init();

		inline static std::shared_ptr<spdlog::logger> &getCoreLogger() { return core_logger; }
		inline static std::shared_ptr<spdlog::logger> &getAppLogger() { return app_logger; }
		inline static void add_singk(const spdlog::sink_ptr &sink) { sinks.push_back(sink); }

	private:
		static std::vector<spdlog::sink_ptr> sinks;
		static std::shared_ptr<spdlog::logger> core_logger;
		static std::shared_ptr<spdlog::logger> app_logger;
	};
}		 // namespace geg

#define GEG_CORE_TRACE(...) geg::Logger::getCoreLogger()->trace(__VA_ARGS__)
#define GEG_CORE_INFO(...)	geg::Logger::getCoreLogger()->info(__VA_ARGS__)
#define GEG_CORE_WARN(...)	geg::Logger::getCoreLogger()->warn(__VA_ARGS__)
#define GEG_CORE_ERROR(...) geg::Logger::getCoreLogger()->error(__VA_ARGS__)
#define GEG_CORE_FETAL(...) geg::Logger::getCoreLogger()->fetal(__VA_ARGS__)

#define GEG_TRACE(...) geg::Logger::getAppLogger()->trace(__VA_ARGS__)
#define GEG_INFO(...)	 geg::Logger::getAppLogger()->info(__VA_ARGS__)
#define GEG_WARN(...)	 geg::Logger::getAppLogger()->warn(__VA_ARGS__)
#define GEG_ERROR(...) geg::Logger::getAppLogger()->error(__VA_ARGS__)
#define GEG_FETAL(...) geg::Logger::getAppLogger()->fetal(__VA_ARGS__)
