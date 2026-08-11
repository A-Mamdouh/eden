#include "Eden/Systems/InputSystem/InputSystem.hpp"

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

void InputSystem::update(double /*dt*/) {
  int numKeys = 0;
  const Uint8 *keyboardState = SDL_GetKeyboardState(&numKeys);

  if (previousKeys_.size() != static_cast<std::size_t>(numKeys)) {
    previousKeys_.assign(static_cast<std::size_t>(numKeys), false);
    currentKeys_.assign(static_cast<std::size_t>(numKeys), false);
  }
  previousKeys_ = currentKeys_;

  SDL_PumpEvents();

  for (int i = 0; i < numKeys; ++i) {
    currentKeys_[static_cast<std::size_t>(i)] = keyboardState[i] != 0;
  }

  int deltaX = 0;
  int deltaY = 0;
  SDL_GetRelativeMouseState(&deltaX, &deltaY);
  mouseDelta_ = Vec2{static_cast<float>(deltaX), static_cast<float>(deltaY)};
}

bool InputSystem::isKeyDown(Key key) const {
  const auto scancode = static_cast<std::size_t>(toScancode(key));
  return scancode < currentKeys_.size() && currentKeys_[scancode];
}

bool InputSystem::isKeyPressed(Key key) const {
  const auto scancode = static_cast<std::size_t>(toScancode(key));
  return scancode < currentKeys_.size() && currentKeys_[scancode] && !previousKeys_[scancode];
}

void InputSystem::setMouseCaptured(bool captured) {
  SDL_SetRelativeMouseMode(captured ? SDL_TRUE : SDL_FALSE);
  mouseCaptured_ = captured;
}

} // namespace Eden
