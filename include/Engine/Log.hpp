//#pragma once

#ifndef EDEN_ENGINE_LOG_HPP
#define EDEN_ENGINE_LOG_HPP

#include "Engine/EdenConfig.hpp"

#include <cstdlib>
#include <memory>
#include <spdlog/spdlog.h>

namespace Eden
{

/**
 * Central logging facility for the engine.
 *
 * Wraps spdlog so that the rest of the codebase only depends on this header.
 */
class Log
{
public:
    /**
     * Initialize the logging system.
     *
     * Safe to call multiple times; subsequent calls are ignored.
     */
    static void init();

    /**
     * Access the core engine logger.
     */
    static std::shared_ptr<spdlog::logger>& core() noexcept;

private:
    static std::shared_ptr<spdlog::logger> coreLogger_;
};

} // namespace Eden

// Convenience logging macros.

#define EDEN_CORE_TRACE(...) ::Eden::Log::core()->trace(__VA_ARGS__)
#define EDEN_CORE_DEBUG(...) ::Eden::Log::core()->debug(__VA_ARGS__)
#define EDEN_CORE_INFO(...)  ::Eden::Log::core()->info(__VA_ARGS__)
#define EDEN_CORE_WARN(...)  ::Eden::Log::core()->warn(__VA_ARGS__)
#define EDEN_CORE_ERROR(...) ::Eden::Log::core()->error(__VA_ARGS__)
#define EDEN_CORE_CRITICAL(...) ::Eden::Log::core()->critical(__VA_ARGS__)

// Simple assertion macro that logs failures before aborting.

#define EDEN_ASSERT(expr, msg)                                                     \
    do                                                                            \
    {                                                                             \
        if (!(expr))                                                              \
        {                                                                         \
            EDEN_CORE_CRITICAL("Assertion failed: {} (expr: {})", (msg), #expr);  \
            std::abort();                                                         \
        }                                                                         \
    } while (false)

#endif // EDEN_ENGINE_LOG_HPP
