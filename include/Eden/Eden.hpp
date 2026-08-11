#pragma once

#include "Eden/Core/Engine.hpp"
#include "Eden/Services/ConfigService/Config.hpp"

/// Single umbrella header for applications embedding Eden: pulls in
/// Engine and the config types needed to construct one.
namespace Eden {
/// Config type an application fills out and passes to Engine's constructor.
using AppConfig = Config::ApplicationConfig;
} // namespace Eden

