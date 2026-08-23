#pragma once

#include "Eden/Systems/ISystem.hpp"
#include "Eden/Systems/InputSystem/InputState.hpp"

namespace Eden {

/// Polls SDL's keyboard/mouse state once per frame into its owned
/// InputState, then publishes Events::InputStateUpdatedEvent -- the only
/// channel anything else (ScriptSystem included) uses to reach it, the
/// same way SceneService hands out its Scene via SceneLoadedEvent rather
/// than being depended on directly. See InputState for the actual query
/// API and Events::InputStateUpdatedEvent for how to receive it.
///
/// Deliberately just polling-style state (down / pressed-this-frame,
/// relative mouse motion) rather than a discrete event stream: that
/// covers everything a movement/mouselook script actually needs,
/// without fanning per-keystroke events out to listeners. Quit/resize
/// stay handled where they already were, in RenderSystem's own
/// SDL_PollEvent loop -- polling keyboard/mouse state via
/// SDL_GetKeyboardState()/SDL_GetRelativeMouseState()/SDL_GetMouseState()
/// doesn't drain that same event queue, so the two coexist without
/// racing each other for events.
class InputSystem : public ISystem {
public:
  std::string getName() override { return "Input System"; }
  /// Refreshes state_ from SDL (keys, relative mouse delta, absolute
  /// mouse position), applies any setMouseCaptured() request a
  /// subscriber made against last frame's state_, then publishes
  /// Events::InputStateUpdatedEvent.
  void update(double dt) override;
  void shutdown() override {}

  /// @return This frame's input state. Also reachable, without a direct
  ///         dependency on InputSystem, via Events::InputStateUpdatedEvent.
  InputState &state() { return state_; }
  /// @overload
  const InputState &state() const { return state_; }

private:
  void onInit() override {}

  InputState state_{};
};

} // namespace Eden
