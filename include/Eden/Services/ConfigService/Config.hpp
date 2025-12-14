#pragma once

#include <string>

namespace Eden::Config {
struct WindowConfig {
  int width{1280};
  int height{720};
  bool fullscreen{false};
  bool resizable{true};
  std::string title{"Eden"};
};

struct RenderConfig {
  bool enableValidationLayers{false};
  float targetFrameRate{60.0f};
  bool enableMaxFPS{false};
};

struct JobServiceConfig {
  int numWorkers{4};
};

struct ClockConfig {
  double fixedDt{1.0 / 60.0};    // Simulation step
  double maxFrameDt{0.25};       // Clamp (anti-spiral)
  double timeScale{1.0};         // Slow-mo
};

struct EngineConfig {
  WindowConfig window{};
  RenderConfig render{};
  ClockConfig clock{};
  JobServiceConfig jobs{};
};

struct ApplicationConfig {
  EngineConfig engine{};
};
} 
