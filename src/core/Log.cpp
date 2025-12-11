#include "core/Log.hpp"

#include "EdenConfig.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Eden
{

std::shared_ptr<spdlog::logger> Log::coreLogger_;

void Log::init()
{
    // Ensure a logger exists, then configure it.
    if (!coreLogger_)
    {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sink->set_pattern("[%T] [%^%l%$] %n: %v");

        coreLogger_ = std::make_shared<spdlog::logger>("Eden", sink);
        spdlog::register_logger(coreLogger_);
    }

    coreLogger_->set_level(spdlog::level::trace);

    EDEN_CORE_INFO(
        "Initialized {} {} ({})",
        EDEN_ENGINE_NAME,
        EDEN_ENGINE_VERSION,
        EDEN_ENGINE_DESCRIPTION);
}

std::shared_ptr<spdlog::logger>& Log::core() noexcept
{
    if (!coreLogger_)
    {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sink->set_pattern("[%T] [%^%l%$] %n: %v");

        coreLogger_ = std::make_shared<spdlog::logger>("Eden", sink);
        coreLogger_->set_level(spdlog::level::trace);
        spdlog::register_logger(coreLogger_);
    }

    return coreLogger_;
}

} // namespace Eden
