#pragma once

#include "Eden/Services/ConfigService/Config.hpp"

#include <memory>
#include <vector>

namespace Eden {
class ClockService;
class ConfigService;
class EventService;
class ISystem;
class RenderSystem;

class Engine {
public:
  explicit Engine(const Config::ApplicationConfig &appConfig);
  ~Engine();

  Engine(const Engine &) = delete;
  Engine &operator=(const Engine &) = delete;

  int run();
  void stop();

private:
  void init();
  void shutdown();

  Config::ApplicationConfig config_;
  bool running_{false};

  std::shared_ptr<EventService> eventService_{};
  std::unique_ptr<ConfigService> configService_{};
  std::unique_ptr<ClockService> clockService_{};

  std::vector<std::unique_ptr<ISystem>> systems_{};
  RenderSystem *renderSystem_{nullptr};
};

} // namespace Eden
