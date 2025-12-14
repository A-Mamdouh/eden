#include "Eden/Systems/RenderSystem/RenderSystem.hpp"

#include "Eden/Systems/RenderSystem/Vulkan/VulkanRenderer.hpp"

#include <SDL.h>
#include <spdlog/spdlog.h>

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
      renderer_->drawFrame();
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
