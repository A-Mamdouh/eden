#include "Eden/Systems/InputSystem/InputState.hpp"

#include <SDL.h>

namespace Eden {

namespace {

SDL_Scancode toScancode(Key key) {
  switch (key) {
  case Key::W:
    return SDL_SCANCODE_W;
  case Key::A:
    return SDL_SCANCODE_A;
  case Key::S:
    return SDL_SCANCODE_S;
  case Key::D:
    return SDL_SCANCODE_D;
  case Key::Q:
    return SDL_SCANCODE_Q;
  case Key::E:
    return SDL_SCANCODE_E;
  case Key::Space:
    return SDL_SCANCODE_SPACE;
  case Key::LeftShift:
    return SDL_SCANCODE_LSHIFT;
  case Key::LeftControl:
    return SDL_SCANCODE_LCTRL;
  case Key::Escape:
    return SDL_SCANCODE_ESCAPE;
  case Key::Up:
    return SDL_SCANCODE_UP;
  case Key::Down:
    return SDL_SCANCODE_DOWN;
  case Key::Left:
    return SDL_SCANCODE_LEFT;
  case Key::Right:
    return SDL_SCANCODE_RIGHT;
  }
  return SDL_SCANCODE_UNKNOWN;
}

} // namespace

bool InputState::isKeyDown(Key key) const {
  const auto scancode = static_cast<std::size_t>(toScancode(key));
  return scancode < currentKeys_.size() && currentKeys_[scancode];
}

bool InputState::isKeyPressed(Key key) const {
  const auto scancode = static_cast<std::size_t>(toScancode(key));
  return scancode < currentKeys_.size() && currentKeys_[scancode] && !previousKeys_[scancode];
}

} // namespace Eden
