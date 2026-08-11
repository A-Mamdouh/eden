#include "Eden/Systems/RenderSystem/RenderSystem.hpp"

#include "Eden/Services/SceneService/Components.hpp"
#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Systems/RenderSystem/Renderer.hpp"
#include "Systems/RenderSystem/Vulkan/VulkanRendererFactory.hpp"

#include <SDL.h>
#include <spdlog/spdlog.h>

#include <array>
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

  renderer_ = createVulkanRenderer(window_, renderConfig_.enableValidationLayers);

  createPrimitiveMeshes();
}

void RenderSystem::createPrimitiveMeshes() {
  const std::array<Vertex, 3> triangleVertices{
      Vertex{Vec3{0.0f, 0.5f, 0.0f}, Color{1.0f, 0.0f, 0.0f, 1.0f}},
      Vertex{Vec3{0.5f, -0.5f, 0.0f}, Color{0.0f, 1.0f, 0.0f, 1.0f}},
      Vertex{Vec3{-0.5f, -0.5f, 0.0f}, Color{0.0f, 0.0f, 1.0f, 1.0f}},
  };
  triangleMesh_ = renderer_->createMesh(MeshDesc{triangleVertices});

  const std::array<Vertex, 6> quadVertices{
      Vertex{Vec3{-0.5f, 0.5f, 0.0f}, Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{Vec3{0.5f, 0.5f, 0.0f}, Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{Vec3{0.5f, -0.5f, 0.0f}, Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{Vec3{0.5f, -0.5f, 0.0f}, Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{Vec3{-0.5f, -0.5f, 0.0f}, Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{Vec3{-0.5f, 0.5f, 0.0f}, Color{1.0f, 1.0f, 1.0f, 1.0f}},
  };
  quadMesh_ = renderer_->createMesh(MeshDesc{quadVertices});
}

MeshHandle RenderSystem::resolvePrimitive(PrimitiveShape shape) const {
  switch (shape) {
  case PrimitiveShape::Triangle:
    return triangleMesh_;
  case PrimitiveShape::Quad:
    return quadMesh_;
  }
  return {};
}

RenderFrame RenderSystem::buildFrameFromScene() const {
  RenderFrame frame{};
  frame.clearColor = Color{0.05f, 0.05f, 0.08f, 1.0f};

  Scene *scene = sceneService_.activeScene();
  if (!scene) {
    return frame;
  }

  auto &registry = scene->getRegistry();
  for (const auto entity : registry.view<Renderable, WorldTransform>()) {
    const auto &renderable = registry.get<Renderable>(entity);
    const auto &worldTransform = registry.get<WorldTransform>(entity);

    DrawCommand draw{};
    draw.mesh = resolvePrimitive(renderable.shape);
    draw.transform = worldTransform.matrix;
    draw.tint = renderable.tint;
    draw.useVertexColor = renderable.useVertexColor;
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
