#include "Eden/Core/Engine.hpp"

#include "Eden/Services/ClockService/ClockService.hpp"
#include "Eden/Services/ConfigService/ConfigService.hpp"
#include "Eden/Services/EventService/EventService.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Systems/RenderSystem/RenderSystem.hpp"
#include "Eden/Systems/ScriptSystem/ScriptSystem.hpp"
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

  // Scripts run first so any component writes they make (Transform,
  // Renderable, ...) are visible to TransformSystem/RenderSystem the
  // same frame, not one frame late.
  auto scriptSystem = std::make_unique<ScriptSystem>(*sceneService_);
  scriptSystem->init(eventService_);
  systems_.push_back(std::move(scriptSystem));

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

Model Engine::loadModel(const std::string &path) { return renderSystem_->loadModel(path); }

MeshHandle Engine::createMesh(const MeshDesc &desc) { return renderSystem_->createMesh(desc); }

void Engine::destroyMesh(MeshHandle handle) { renderSystem_->destroyMesh(handle); }

TextureHandle Engine::createTexture(const TextureDesc &desc) {
  return renderSystem_->createTexture(desc);
}

void Engine::destroyTexture(TextureHandle handle) { renderSystem_->destroyTexture(handle); }

MaterialHandle Engine::createMaterial(const Material &desc) {
  return renderSystem_->createMaterial(desc);
}

void Engine::destroyMaterial(MaterialHandle handle) { renderSystem_->destroyMaterial(handle); }

void Engine::setActiveCamera(Entity camera) { renderSystem_->setActiveCamera(camera); }

Entity Engine::activeCamera() const { return renderSystem_->activeCamera(); }

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
