#pragma once

#include <cstdint>

namespace Eden {

struct ClearColor {
  float r{0.0f};
  float g{0.0f};
  float b{0.0f};
  float a{1.0f};
};

class Renderer {
public:
  virtual ~Renderer() = default;

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  Renderer(Renderer &&) noexcept = delete;
  Renderer &operator=(Renderer &&) noexcept = delete;

  virtual void drawFrame() = 0;
  virtual void requestResize(std::uint32_t width, std::uint32_t height) = 0;
  virtual void setClearColor(ClearColor color) = 0;

protected:
  Renderer() = default;
};

} // namespace Eden
