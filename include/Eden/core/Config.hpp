#pragma once

#ifndef EDEN_ENGINE_CORE_CONFIG_HPP
#define EDEN_ENGINE_CORE_CONFIG_HPP

#include <string>

namespace Eden {

struct WindowConfig {
  unsigned int width{1280};
  unsigned int height{720};
  std::string title{"Eden"};
  bool resizable{true};
};

struct RenderConfig {
  bool enableValidationLayers{false};
  float targetFrameRate{60.0f};
  bool fixedTimestep{false};
};

struct EngineConfig {
  WindowConfig window{};
  RenderConfig render{};
};

} // namespace Eden

#endif // EDEN_ENGINE_CORE_CONFIG_HPP
