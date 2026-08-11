#include "Eden/Core/Engine.hpp"

#include "Eden/Services/ClockService/ClockService.hpp"
#include "Eden/Services/ConfigService/ConfigService.hpp"
#include "Eden/Services/EventService/EventService.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Systems/RenderSystem/RenderSystem.hpp"
#include "Eden/Systems/TransformSystem.hpp"

#include <spdlog/spdlog.h>

namespace Eden {

Engine::Engine(const Config::ApplicationConfig &appConfig) : config_{appConfig} {
  init();
}

Engine::~Engine() { shutdown(); }

void Engine::init() {
  eventService_ = std::make_shared<EventService>();
  eventService_->init(eventService_);

  configService_ = std::make_unique<ConfigService>(config_);
  configService_->init(eventService_);

  clockService_ = std::make_unique<ClockService>(config_.engine.clock);
  clockService_->init(eventService_);

  sceneService_ = std::make_unique<SceneService>();
  sceneService_->init(eventService_);

  auto transformSystem = std::make_unique<TransformSystem>(*sceneService_);
  transformSystem->init(eventService_);
  systems_.push_back(std::move(transformSystem));

  auto renderSystem = std::make_unique<RenderSystem>(config_.engine.window,
                                                     config_.engine.render, *sceneService_);
  renderSystem_ = renderSystem.get();
  renderSystem_->init(eventService_);
  systems_.push_back(std::move(renderSystem));

  running_ = true;
}

int Engine::run() {
  while (running_) {
    const double dt = clockService_ ? clockService_->tick() : 0.0;

    for (auto &system : systems_) {
      system->update(dt);
    }

    if (renderSystem_ && renderSystem_->quitRequested()) {
      stop();
    }
  }

  shutdown();
  spdlog::info("Engine shutdown complete");
  return 0;
}

void Engine::stop() { running_ = false; }

void Engine::loadScene(std::unique_ptr<Scene> scene) {
  sceneService_->loadScene(std::move(scene));
}

void Engine::shutdown() {
  if (!eventService_) {
    return;
  }

  for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
    (*it)->shutdown();
  }
  systems_.clear();
  renderSystem_ = nullptr;

  if (clockService_) {
    clockService_->stop();
  }
  if (configService_) {
    configService_->stop();
  }
  if (sceneService_) {
    sceneService_->stop();
  }

  clockService_.reset();
  configService_.reset();
  sceneService_.reset();

  eventService_->stop();
  eventService_.reset();
}

} // namespace Eden
