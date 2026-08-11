#include "Eden/Systems/RenderSystem/RenderSystem.hpp"

#include "Eden/Systems/RenderSystem/Vulkan/VulkanRenderer.hpp"

#include <SDL.h>
#include <spdlog/spdlog.h>

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace Eden {

RenderSystem::RenderSystem(Config::WindowConfig windowConfig,
                           Config::RenderConfig renderConfig)
    : windowConfig_{std::move(windowConfig)}, renderConfig_{std::move(renderConfig)} {}

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

  renderer_ = std::make_unique<VulkanRenderer>(VulkanRenderer::CreateInfo{
      .window = window_, .enableValidationLayers = renderConfig_.enableValidationLayers});

  createDemoMeshes();
}

void RenderSystem::createDemoMeshes() {
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

RenderFrame RenderSystem::buildDemoFrame(double elapsedTime) const {
  RenderFrame frame{};
  frame.clearColor = Color{0.05f, 0.05f, 0.08f, 1.0f};

  DrawCommand triangleDraw{};
  triangleDraw.mesh = triangleMesh_;
  triangleDraw.transform = glm::translate(Mat4(1.0f), Vec3{-0.6f, 0.0f, 0.0f});
  triangleDraw.useVertexColor = true;
  frame.commands.push_back(triangleDraw);

  const float pulse = static_cast<float>(0.5 + 0.5 * std::sin(elapsedTime));
  DrawCommand quadDraw{};
  quadDraw.mesh = quadMesh_;
  quadDraw.transform = glm::translate(Mat4(1.0f), Vec3{0.6f, 0.0f, 0.0f});
  quadDraw.tint = Color{pulse, 0.3f, 1.0f - pulse, 1.0f};
  quadDraw.useVertexColor = false;
  frame.commands.push_back(quadDraw);

  return frame;
}

void RenderSystem::update(double dt) {
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
      elapsedTime_ += dt;
      renderer_->renderFrame(buildDemoFrame(elapsedTime_));
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
