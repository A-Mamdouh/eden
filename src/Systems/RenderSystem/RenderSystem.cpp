#include "Eden/Systems/RenderSystem/RenderSystem.hpp"

#include "Eden/Services/SceneService/Components.hpp"
#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Systems/RenderSystem/Frustum.hpp"
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

MeshHandle RenderSystem::createMesh(const MeshDesc &desc) {
  const MeshHandle handle = renderer_->createMesh(desc);
  if (handle.valid()) {
    const std::uint32_t index = handle.id - 1;
    if (index >= meshBounds_.size()) {
      meshBounds_.resize(index + 1);
    }
    meshBounds_[index] = MeshBoundsSlot{computeBounds(desc.vertices), handle.generation};
  }
  return handle;
}

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

Model RenderSystem::loadModel(const std::string &path) { return loadGltfModel(path, *this); }

void RenderSystem::setActiveCamera(Entity camera) { activeCamera_ = camera.handle(); }

Entity RenderSystem::activeCamera() const {
  Scene *scene = sceneService_.activeScene();
  if (!scene) {
    return Entity{};
  }
  return Entity{activeCamera_, &scene->getRegistry()};
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

const AABB *RenderSystem::resolveMeshBounds(MeshHandle mesh) const {
  if (!mesh.valid()) {
    return nullptr;
  }
  const std::uint32_t index = mesh.id - 1;
  if (index >= meshBounds_.size()) {
    return nullptr;
  }
  const MeshBoundsSlot &slot = meshBounds_[index];
  if (slot.generation != mesh.generation) {
    return nullptr;
  }
  return &slot.bounds;
}

DrawCommand RenderSystem::buildDrawCommand(MeshHandle mesh, MaterialHandle material,
                                           const Mat4 &transform,
                                           const TintOverride *tintOverride) const {
  DrawCommand draw{};
  draw.mesh = mesh;
  draw.transform = transform;

  if (const Material *resolvedMaterial = resolveMaterial(material)) {
    draw.tint = resolvedMaterial->tint;
    draw.useVertexColor = resolvedMaterial->useVertexColor;
    draw.texture = resolvedMaterial->texture;
  }

  if (tintOverride) {
    draw.tint = tintOverride->tint;
    draw.useVertexColor = false;
  }

  return draw;
}

CameraDesc RenderSystem::resolveCamera(Scene &scene) const {
  const float aspect = windowHeight_ > 0
                            ? static_cast<float>(windowWidth_) / static_cast<float>(windowHeight_)
                            : 1.0f;

  auto &registry = scene.getRegistry();
  if (registry.valid(activeCamera_) && registry.all_of<Camera>(activeCamera_)) {
    const auto &camera = registry.get<Camera>(activeCamera_);
    return CameraDesc{camera.viewMatrix(), camera.projectionMatrix(aspect)};
  }

  // No camera set (or it doesn't resolve in this scene): fall back to an
  // aspect-corrected orthographic projection (identity view) instead of a
  // bare identity projection, so scenes authored without a camera still
  // render undistorted and fully in view regardless of window aspect ratio.
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
  const Frustum frustum{frame.camera.projection * frame.camera.view};

  auto &registry = scene->getRegistry();
  for (const auto entity : registry.view<Renderable, WorldTransform>()) {
    const auto &renderable = registry.get<Renderable>(entity);
    const auto &worldTransform = registry.get<WorldTransform>(entity);
    const AABB *bounds = resolveMeshBounds(renderable.mesh);
    if (bounds && !frustum.intersects(*bounds, worldTransform.matrix)) {
      continue;
    }
    frame.commands.push_back(buildDrawCommand(renderable.mesh, renderable.material, worldTransform.matrix,
                                               registry.try_get<TintOverride>(entity)));
  }

  for (const auto entity : registry.view<Model, WorldTransform>()) {
    const auto &model = registry.get<Model>(entity);
    const auto &worldTransform = registry.get<WorldTransform>(entity);
    const auto *tintOverride = registry.try_get<TintOverride>(entity);
    for (const ModelPart &part : model.parts) {
      const Mat4 partTransform = worldTransform.matrix * part.localTransform;
      const AABB *bounds = resolveMeshBounds(part.mesh);
      if (bounds && !frustum.intersects(*bounds, partTransform)) {
        continue;
      }
      frame.commands.push_back(buildDrawCommand(part.mesh, part.material, partTransform, tintOverride));
    }
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
