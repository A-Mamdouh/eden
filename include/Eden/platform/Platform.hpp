#pragma once

#ifndef EDEN_ENGINE_PLATFORM_HPP
#define EDEN_ENGINE_PLATFORM_HPP

#include "Input.hpp"
#include "Window.hpp"
#include "Eden/core/System.hpp"

#include <memory>

namespace Eden {
class Platform : public ISystem {
public:

  ~Platform() = default;
  Platform(const Platform &) = delete;
  Platform &operator=(const Platform &) = delete;
  Platform(Platform &&) noexcept = delete;
  Platform &operator=(Platform &&) noexcept = delete;

  static Platform &getInstance();
  void init(const ConfigurationService &config) override;
  void update(float /*deltaTime*/) override {}
  void shutdown() override;
  std::shared_ptr<Window> getWindow();
  std::shared_ptr<Input> getInput();

private:
  Platform();
  static std::unique_ptr<Platform> instance_;
  std::shared_ptr<Window> window_;
  std::shared_ptr<Input> input_;
};
} // namespace Eden
#endif
