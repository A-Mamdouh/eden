#pragma once

#include <string>

namespace Eden::Config {
  struct WindowConfig {
  int height;
  int width;
  bool fullscreen;
  bool resizable;
  std::string title;
};

struct RendererConfig {
  bool enableValidation{true};
  float targetFrameRate{60.0f};
  bool enableMaxFPS{true};
};

struct JobServiceConfig {
  int numWorkers{4};
};

struct Clock {
  double fixedDt = 1.0f / 60.0f; // Simulation step
  double maxFrameDt = 0.25f; // Clamp (anti-spiral)
  double timeScale = 1.0f; // Slow-mo
};

struct EngineConfig {
  WindowConfig window;
  RendererConfig renderer;
};

struct ApplicationConfig {
  EngineConfig engine;
};
}