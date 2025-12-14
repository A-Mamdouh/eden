#pragma once

#include "Eden/Systems/ISystem.hpp"
#include "Eden/Services/ConfigService/Config.hpp"

struct SDL_Window;

namespace Eden {

class Renderer;

class RenderSystem : public ISystem {

  public:
  RenderSystem(Config::WindowConfig windowConfig, Config::RenderConfig renderConfig);
  ~RenderSystem() override;

  std::string getName() override { return "Render System"; }
  void update(double dt) override;
  void shutdown() override;

  bool quitRequested() const noexcept { return quitRequested_; }

  private:
  void onInit() override;

  Config::WindowConfig windowConfig_;
  Config::RenderConfig renderConfig_;

  SDL_Window *window_{nullptr};
  std::unique_ptr<Renderer> renderer_{nullptr};
  bool quitRequested_{false};

};

} // namespace Eden
