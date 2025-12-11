#pragma once

#ifndef EDEN_ENGINE_APPLICATION_HPP
#define EDEN_ENGINE_APPLICATION_HPP

#include "Engine.hpp"
#include "Config.hpp"
#include <Eden/scene/Scene.hpp>

#include <memory>

namespace Eden {

struct AppConfig {
  EngineConfig engine;
};

/**
 * High-level application wrapper around the core Engine.
 *
 * Users derive from Application and provide an initial Scene.
 */
class Application {
public:
  static Application &getInstance();
  static void setInstance(std::unique_ptr<Application> instance);
  AppConfig getConfig() { return config_; }

  virtual ~Application();
  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&) noexcept = delete;
  Application &operator=(Application &&) noexcept = delete;

  explicit Application(const AppConfig &config = {});
  explicit Application(const EngineConfig &config);

  void exit();

  int run();

protected:
  virtual void onInit() {}
  virtual void onShutdown() {}
  virtual void onWindowResized(unsigned int /*width*/,
                               unsigned int /*height*/) {}
  virtual std::unique_ptr<Scene> createInitialScene() = 0;
  void quit() noexcept { Engine::getInstance().stop(); }

private:
  static std::unique_ptr<Application> instance_;
  AppConfig config_{};
};

} // namespace Eden

#endif // EDEN_ENGINE_APPLICATION_HPP
