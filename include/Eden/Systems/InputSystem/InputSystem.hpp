#pragma once

#include "Eden/Core/Math.hpp"
#include "Eden/Systems/ISystem.hpp"
#include "Eden/Systems/InputSystem/Key.hpp"

#include <vector>

namespace Eden {

/// Polls SDL's keyboard/mouse state once per frame and exposes it as
/// simple queries -- ScriptSystem passes this to every ScriptBehaviour,
/// so scripts read input the same way they read any other engine state,
/// with no direct SDL dependency.
///
/// Deliberately just polling-style state (down / pressed-this-frame,
/// relative mouse motion) rather than an event stream: that covers
/// everything a movement/mouselook script actually needs, and it avoids
/// having to fan discrete events out to listeners the way EventService
/// does. Quit/resize stay handled where they already were, in
/// RenderSystem's own SDL_PollEvent loop -- polling keyboard/mouse state
/// via SDL_GetKeyboardState()/SDL_GetRelativeMouseState() doesn't drain
/// that same event queue, so the two coexist without racing each other
/// for events.
class InputSystem : public ISystem {
public:
  std::string getName() override { return "Input System"; }
  void update(double dt) override;
  void shutdown() override {}

  /// @return True for every frame `key` is held down.
  bool isKeyDown(Key key) const;
  /// @return True only on the frame `key` transitions from up to down.
  bool isKeyPressed(Key key) const;

  /// @return Mouse motion since the last update(), in pixels. Only a
  ///         clean look delta while mouseCaptured() is true -- otherwise
  ///         it's motion bounded by the screen edges, not a free delta.
  Vec2 mouseDelta() const { return mouseDelta_; }

  /// Hides the cursor and reports its motion as an unbounded delta (via
  /// mouseDelta()) instead of a screen-edge-clamped position -- what a
  /// mouselook camera wants.
  void setMouseCaptured(bool captured);
  bool mouseCaptured() const { return mouseCaptured_; }

private:
  void onInit() override {}

  std::vector<bool> previousKeys_{};
  std::vector<bool> currentKeys_{};
  Vec2 mouseDelta_{0.0f, 0.0f};
  bool mouseCaptured_{false};
};

} // namespace Eden
