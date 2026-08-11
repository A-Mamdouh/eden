#include "Eden/Systems/RenderSystem/RenderSystem.hpp"

#include "Eden/Services/SceneService/Components.hpp"
#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Systems/RenderSystem/Renderer.hpp"
#include "Systems/RenderSystem/GltfLoader.hpp"
#include "Systems/RenderSystem/Vulkan/VulkanRendererFactory.hpp"

#include <SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <utility>

namespace Eden {

RenderSystem::RenderSystem(Config::WindowConfig windowConfig,
                           Config::RenderConfig renderConfig, SceneService &sceneService)
    : windowConfig_{std::move(windowConfig)}, renderConfig_{std::move(renderConfig)},
      sceneService_{sceneService} {}

RenderSystem::~RenderSystem() { shutdown(); }

void RenderSystem::onInit() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error(SDL_GetError());
  }

  std::uint32_t windowFlags = SDL_WINDOW_VULKAN;
  if (windowConfig_.resizable) {
    windowFlags |= SDL_WINDOW_RESIZABLE;
  }
  if (windowConfig_.fullscreen) {
    windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  }

  window_ = SDL_CreateWindow(windowConfig_.title.c_str(), SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, windowConfig_.width,
                             windowConfig_.height, windowFlags);

  if (!window_) {
    throw std::runtime_error(SDL_GetError());
  }

  windowWidth_ = windowConfig_.width;
  windowHeight_ = windowConfig_.height;

  renderer_ = createVulkanRenderer(window_, renderConfig_.enableValidationLayers);
}

MeshHandle RenderSystem::createMesh(const MeshDesc &desc) { return renderer_->createMesh(desc); }

void RenderSystem::destroyMesh(MeshHandle handle) { renderer_->destroyMesh(handle); }

TextureHandle RenderSystem::createTexture(const TextureDesc &desc) {
  return renderer_->createTexture(desc);
}

void RenderSystem::destroyTexture(TextureHandle handle) { renderer_->destroyTexture(handle); }

MaterialHandle RenderSystem::createMaterial(const Material &desc) {
  MaterialSlot slot{};
  slot.material = desc;
  slot.alive = true;

  std::uint32_t index{};
  if (!freeMaterialSlots_.empty()) {
    index = freeMaterialSlots_.back();
    freeMaterialSlots_.pop_back();
    slot.generation = materials_[index].generation + 1;
    materials_[index] = slot;
  } else {
    index = static_cast<std::uint32_t>(materials_.size());
    slot.generation = 1;
    materials_.push_back(slot);
  }

  return MaterialHandle{index + 1, materials_[index].generation};
}

void RenderSystem::destroyMaterial(MaterialHandle handle) {
  if (!handle.valid()) {
    return;
  }
  const std::uint32_t index = handle.id - 1;
  if (index >= materials_.size()) {
    return;
  }

  MaterialSlot &slot = materials_[index];
  if (!slot.alive || slot.generation != handle.generation) {
    return;
  }

  slot.alive = false;
  freeMaterialSlots_.push_back(index);
}

void RenderSystem::loadModel(const std::string &path, Scene &scene) {
  loadGltfModel(path, *this, scene);
}

const Material *RenderSystem::resolveMaterial(MaterialHandle handle) const {
  if (!handle.valid()) {
    return nullptr;
  }
  const std::uint32_t index = handle.id - 1;
  if (index >= materials_.size()) {
    return nullptr;
  }
  const MaterialSlot &slot = materials_[index];
  if (!slot.alive || slot.generation != handle.generation) {
    return nullptr;
  }
  return &slot.material;
}

CameraDesc RenderSystem::resolveCamera(Scene &scene) const {
  const float aspect = windowHeight_ > 0
                            ? static_cast<float>(windowWidth_) / static_cast<float>(windowHeight_)
                            : 1.0f;

  auto &registry = scene.getRegistry();
  for (const auto entity : registry.view<Camera>()) {
    const auto &camera = registry.get<Camera>(entity);
    if (!camera.active) {
      continue;
    }
    return CameraDesc{camera.viewMatrix(), camera.projectionMatrix(aspect)};
  }

  // No active Camera entity: fall back to an aspect-corrected orthographic
  // projection (identity view) instead of a bare identity projection, so
  // scenes authored without a Camera still render undistorted and fully in
  // view regardless of window aspect ratio.
  return CameraDesc{Mat4{1.0f}, glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f)};
}

RenderFrame RenderSystem::buildFrameFromScene() const {
  RenderFrame frame{};
  frame.clearColor = Color{0.05f, 0.05f, 0.08f, 1.0f};

  Scene *scene = sceneService_.activeScene();
  if (!scene) {
    return frame;
  }

  frame.camera = resolveCamera(*scene);

  auto &registry = scene->getRegistry();
  for (const auto entity : registry.view<Renderable, WorldTransform>()) {
    const auto &renderable = registry.get<Renderable>(entity);
    const auto &worldTransform = registry.get<WorldTransform>(entity);

    DrawCommand draw{};
    draw.mesh = renderable.mesh;
    draw.transform = worldTransform.matrix;

    if (const Material *material = resolveMaterial(renderable.material)) {
      draw.tint = material->tint;
      draw.useVertexColor = material->useVertexColor;
      draw.texture = material->texture;
    }

    if (const auto *tintOverride = registry.try_get<TintOverride>(entity)) {
      draw.tint = tintOverride->tint;
      draw.useVertexColor = false;
    }

    frame.commands.push_back(draw);
  }

  return frame;
}

void RenderSystem::update(double /*dt*/) {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    switch (event.type) {
    case SDL_QUIT:
      quitRequested_ = true;
      break;
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
        quitRequested_ = true;
      }
      if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
          event.window.event == SDL_WINDOWEVENT_RESIZED) {
        windowWidth_ = event.window.data1;
        windowHeight_ = event.window.data2;
        if (renderer_) {
          renderer_->requestResize(static_cast<std::uint32_t>(event.window.data1),
                                   static_cast<std::uint32_t>(event.window.data2));
        }
      }
      break;
    default:
      break;
    }
  }

  if (quitRequested_) {
    return;
  }

  try {
    if (renderer_) {
      renderer_->renderFrame(buildFrameFromScene());
    }
  } catch (const std::exception &e) {
    spdlog::error("Renderer error: {}", e.what());
    quitRequested_ = true;
  }
}

void RenderSystem::shutdown() {
  renderer_.reset();

  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }

  SDL_Quit();
}

} // namespace Eden
