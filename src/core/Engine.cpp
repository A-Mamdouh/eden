#include <Eden/core/Engine.hpp>
#include <Eden/core/Application.hpp>
#include <Eden/core/EventSystem.hpp>
#include <Eden/core/Configuration.hpp>
#include <Eden/core/System.hpp>
#include <Eden/platform/Platform.hpp>
#include <Eden/render/RenderSystem.hpp>
#include <Eden/script/ScriptSystem.hpp>
#include <Eden/scene/SceneManager.hpp>
#include "core/Log.hpp"

#include <chrono>
#include <memory>
#include <thread>

namespace Eden {

std::unique_ptr<Engine> Engine::instance_ = nullptr;

Engine &Engine::getInstance() {
  if (instance_ == nullptr) {
    auto config = Application::getInstance().getConfig().engine;
    instance_ = std::unique_ptr<Engine>(new Engine(config));
  }
  return *instance_;
}

Engine::Engine(const EngineConfig &config)
    : configService_(&ConfigurationService::getInstance()) {
  configService_->set(config);
  platform_ = &Platform::getInstance();
  renderSystem_ = &RenderSystem::getInstance();
  scriptSystem_ = &ScriptSystem::getInstance();
  sceneManager_ = &SceneManager::getInstance();

  registerSystem(*platform_);
  registerSystem(EventSystem::getInstance());
  registerSystem(*scriptSystem_);
  registerSystem(*renderSystem_);
}

ConfigurationService &Engine::getConfigService() noexcept {
  return *configService_;
}

const ConfigurationService &Engine::getConfigService() const noexcept {
  return *configService_;
}

Platform &Engine::platform() noexcept { return *platform_; }
RenderSystem &Engine::renderSystem() noexcept { return *renderSystem_; }
ScriptSystem &Engine::scriptSystem() noexcept { return *scriptSystem_; }
SceneManager &Engine::sceneManager() noexcept { return *sceneManager_; }

void Engine::registerSystem(ISystem &system) {
  systems_.push_back(&system);
}

void Engine::run() {
  running_ = true;
  accumulator_ = 0.0f;

  for (auto *system : systems_) {
    system->init(*configService_);
  }

  using clock = std::chrono::high_resolution_clock;
  auto lastTime = clock::now();

  EDEN_CORE_INFO(
      "Engine main loop started (fixed timestep: {}, target FPS: {})",
      configService_->get().render.fixedTimestep,
      configService_->get().render.targetFrameRate);

  while (running_) {
    const auto now = clock::now();
    const std::chrono::duration<float> delta = now - lastTime;
    lastTime = now;

    const float frameDelta = delta.count();

    for (auto *system : systems_) {
      system->update(frameDelta);
    }

    if (renderSystem_->shouldClose()) {
      stop();
    }

    const auto &config = configService_->get();
    if (!config.render.fixedTimestep && config.render.targetFrameRate > 0.0f) {
      const auto targetDuration =
          std::chrono::duration<float>(1.0f / config.render.targetFrameRate);
      const auto frameDuration = clock::now() - now;

      if (frameDuration < targetDuration) {
        std::this_thread::sleep_for(targetDuration - frameDuration);
      }
    }
  }

  for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
    (*it)->shutdown();
  }

  running_ = false;
  EDEN_CORE_INFO("Engine main loop exited");
}

void Engine::stop() noexcept { running_ = false; }

} // namespace Eden
