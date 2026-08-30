#include "Eden/Systems/RenderSystem/RenderSystem.hpp"

#include "Eden/Services/ConfigService/ConfigServiceEvents.hpp"
#include "Eden/Services/SceneService/Components.hpp"
#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Systems/RenderSystem/Frustum.hpp"
#include "Eden/Systems/RenderSystem/NullRenderer.hpp"
#include "Eden/Systems/RenderSystem/Renderer.hpp"
#include "Systems/RenderSystem/GltfLoader.hpp"
#include "Systems/RenderSystem/Vulkan/VulkanRendererFactory.hpp"

#include <SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <utility>

namespace Eden::Systems {
using namespace Eden::Rendering;
using namespace Eden::Rendering::Components;
using namespace Eden::World;
using namespace Eden::Services;

RenderSystem::RenderSystem(Config::Rendering::RenderConfig renderConfig, SceneService &sceneService)
    : renderConfig_{std::move(renderConfig)}, sceneService_{sceneService} {}

RenderSystem::~RenderSystem() { shutdown(); }

void RenderSystem::onInit() {
  // Without this, Windows treats the process as DPI-unaware and silently
  // upscales the whole window to match the display's scale factor (e.g. a
  // requested 1920x1080 window physically renders larger than that on a
  // 4K display at 150%/200% scaling) -- must be set before SDL_Init. Using
  // "system" rather than "permonitorv2": RenderSystem doesn't handle a
  // live per-monitor DPI change (WM_DPICHANGED) if the window is dragged
  // to a differently-scaled monitor, so per-monitor awareness would
  // promise more than this actually implements today.
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error(SDL_GetError());
  }

  createWindow();
  createRenderer();

  configListener_ = getEventService()->subscribe<Events::ConfigUpdatedEvent>(
      [this](const Events::ConfigUpdatedEvent &event) { onConfigUpdated(event); });
}

void RenderSystem::createWindow() {
  std::uint32_t windowFlags = SDL_WINDOW_ALLOW_HIGHDPI;
  if (renderConfig_.backend == Config::Rendering::RendererBackend::Vulkan) {
    windowFlags |= SDL_WINDOW_VULKAN;
  }
  if (renderConfig_.window.resizable) {
    windowFlags |= SDL_WINDOW_RESIZABLE;
  }
  switch (renderConfig_.display.screenMode) {
  case Config::Rendering::ScreenMode::Borderless:
    windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    break;
  case Config::Rendering::ScreenMode::Fullscreen:
    windowFlags |= SDL_WINDOW_FULLSCREEN;
    break;
  case Config::Rendering::ScreenMode::Windowed:
  default:
    break;
  }

  window_ = SDL_CreateWindow(renderConfig_.window.title.c_str(), SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, static_cast<int>(renderConfig_.display.width),
                             static_cast<int>(renderConfig_.display.height), windowFlags);

  if (!window_) {
    throw std::runtime_error(SDL_GetError());
  }

  // Read back rather than trust renderConfig_.display.width/height: SDL/the
  // OS can adjust the actual window size (DPI scaling, work-area clamping),
  // so this is what's actually true regardless of what was requested.
  int width = 0;
  int height = 0;
  SDL_GetWindowSize(window_, &width, &height);
  windowWidth_ = width;
  windowHeight_ = height;
}

void RenderSystem::createRenderer() {
  renderer_.reset();

  const RenderSettings settings{renderConfig_.graphics.antiAliasing, renderConfig_.display.vsync};
  switch (renderConfig_.backend) {
  case Config::Rendering::RendererBackend::Null:
    renderer_ = std::make_unique<NullRenderer>();
    break;
  case Config::Rendering::RendererBackend::Vulkan:
  default:
    renderer_ = createVulkanRenderer(window_, renderConfig_.enableValidationLayers, settings);
    break;
  }
}

void RenderSystem::onConfigUpdated(const Events::ConfigUpdatedEvent &event) {
  const Config::Rendering::RenderConfig &oldConfig = renderConfig_;
  const Config::Rendering::RenderConfig &newConfig = event.newConfig->engine.render;

  if (newConfig.window.title != oldConfig.window.title) {
    SDL_SetWindowTitle(window_, newConfig.window.title.c_str());
  }
  if (newConfig.window.resizable != oldConfig.window.resizable) {
    SDL_SetWindowResizable(window_, newConfig.window.resizable ? SDL_TRUE : SDL_FALSE);
  }

  const bool screenChanged = newConfig.display.screenMode != oldConfig.display.screenMode ||
                             newConfig.display.width != oldConfig.display.width ||
                             newConfig.display.height != oldConfig.display.height;
  if (newConfig.display.screenMode != oldConfig.display.screenMode) {
    Uint32 flag = 0;
    switch (newConfig.display.screenMode) {
    case Config::Rendering::ScreenMode::Borderless:
      flag = SDL_WINDOW_FULLSCREEN_DESKTOP;
      break;
    case Config::Rendering::ScreenMode::Fullscreen:
      flag = SDL_WINDOW_FULLSCREEN;
      break;
    case Config::Rendering::ScreenMode::Windowed:
    default:
      flag = 0;
      break;
    }
    SDL_SetWindowFullscreen(window_, flag);
  }
  if (newConfig.display.width != oldConfig.display.width ||
      newConfig.display.height != oldConfig.display.height) {
    SDL_SetWindowSize(window_, static_cast<int>(newConfig.display.width),
                      static_cast<int>(newConfig.display.height));
  }
  if (screenChanged) {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window_, &width, &height);
    windowWidth_ = width;
    windowHeight_ = height;
    if (renderer_) {
      renderer_->requestResize(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    }
  }

  // SDL requires SDL_WINDOW_VULKAN at window creation time, while headless
  // drivers such as "offscreen" reject that flag. Recreate the window when
  // changing backend so Null remains usable without Vulkan window support and
  // a later switch to Vulkan still receives a compatible window.
  if (newConfig.backend != oldConfig.backend) {
    renderer_.reset();
    SDL_DestroyWindow(window_);
    window_ = nullptr;

    renderConfig_ = newConfig;
    createWindow();
    createRenderer();
    return;
  }

  // Validation-layer changes are Vulkan-instance-level -- there's no
  // interface hook to ask a live Renderer about those, so recreate it.
  if (newConfig.enableValidationLayers != oldConfig.enableValidationLayers) {
    renderConfig_ = newConfig;
    createRenderer();
    return;
  }

  if (newConfig.graphics.antiAliasing != oldConfig.graphics.antiAliasing ||
      newConfig.display.vsync != oldConfig.display.vsync) {
    const RenderSettings settings{newConfig.graphics.antiAliasing, newConfig.display.vsync};
    if (renderer_ && renderer_->applySettings(settings) == ApplyResult::RequiresRecreate) {
      renderConfig_ = newConfig;
      createRenderer();
      return;
    }
  }

  renderConfig_ = newConfig;
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
    draw.shadingModel = resolvedMaterial->shadingModel;
    draw.baseColorFactor = resolvedMaterial->baseColorFactor;
    draw.useVertexColor = resolvedMaterial->useVertexColor;
    draw.baseColorTexture = resolvedMaterial->baseColorTexture;
    draw.metallicRoughnessTexture = resolvedMaterial->metallicRoughnessTexture;
    draw.metallicFactor = resolvedMaterial->metallicFactor;
    draw.roughnessFactor = resolvedMaterial->roughnessFactor;
    draw.emissiveTexture = resolvedMaterial->emissiveTexture;
    draw.emissiveFactor = resolvedMaterial->emissiveFactor;
  }

  if (tintOverride) {
    draw.baseColorFactor = tintOverride->tint;
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
    return CameraDesc{camera.viewMatrix(), camera.projectionMatrix(aspect), camera.position};
  }

  // No camera set (or it doesn't resolve in this scene): fall back to an
  // aspect-corrected orthographic projection (identity view) instead of a
  // bare identity projection, so scenes authored without a camera still
  // render undistorted and fully in view regardless of window aspect ratio.
  return CameraDesc{Mat4{1.0f}, glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f), Vec3{0.0f}};
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

  for (const auto entity : registry.view<Light, WorldTransform>()) {
    if (frame.lights.size() >= kMaxLights) {
      spdlog::warn("Scene has more than {} lights; extras are ignored this frame", kMaxLights);
      break;
    }
    const auto &light = registry.get<Light>(entity);
    const Mat4 &worldMatrix = registry.get<WorldTransform>(entity).matrix;

    LightDesc desc{};
    desc.type = light.type;
    desc.color = light.color;
    desc.intensity = light.intensity;
    desc.range = light.range;
    desc.position = Vec3{worldMatrix[3]};
    desc.direction = glm::normalize(Vec3{worldMatrix * Vec4{0.0f, 0.0f, -1.0f, 0.0f}});
    frame.lights.push_back(desc);
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
  if (configListener_) {
    getEventService()->unsubscribe<Events::ConfigUpdatedEvent>(*configListener_);
    configListener_.reset();
  }

  renderer_.reset();

  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }

  SDL_Quit();
}

} // namespace Eden::Systems
