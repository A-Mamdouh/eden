#pragma once

#include "Eden/Core/Math.hpp"
#include "Eden/Systems/InputSystem/Key.hpp"

#include <vector>

namespace Eden {

/// This frame's keyboard/mouse state -- the data half of InputSystem,
/// analogous to how Scene is the data half of SceneService. InputSystem
/// owns one instance, refreshes it from SDL every update(), and hands
/// out a pointer to it via Events::InputStateUpdatedEvent; everything
/// downstream (ScriptBehaviour included) queries/mutates this directly
/// rather than going through InputSystem itself.
///
/// setMouseCaptured() only records the request here -- InputSystem::update()
/// is what reconciles it against real SDL relative-mouse-mode, the same
/// way InputSystem alone still owns every other SDL call.
class InputState {
public:
  /// @return True for every frame `key` is held down.
  bool isKeyDown(Key key) const;
  /// @return True only on the frame `key` transitions from up to down.
  bool isKeyPressed(Key key) const;

  /// @return Mouse motion since the last update(), in pixels. Only a
  ///         clean look delta while mouseCaptured() is true -- otherwise
  ///         it's motion bounded by the screen edges, not a free delta.
  Vec2 mouseDelta() const { return mouseDelta_; }

  /// @return Cursor position in window coordinates, e.g. for UI hit
  ///         testing. SDL doesn't track absolute position while relative
  ///         mouse mode is active, so this stays frozen at the
  ///         pre-capture position for as long as mouseCaptured() is true.
  Vec2 mousePosition() const { return mousePosition_; }

  /// Requests the cursor be hidden and mouseDelta() report unbounded
  /// relative motion instead of a screen-edge-clamped position -- what a
  /// mouselook camera wants. Only records the request; InputSystem::update()
  /// applies it to SDL on the next tick.
  void setMouseCaptured(bool captured) { mouseCaptured_ = captured; }
  bool mouseCaptured() const { return mouseCaptured_; }

private:
  friend class InputSystem;

  std::vector<bool> previousKeys_{};
  std::vector<bool> currentKeys_{};
  Vec2 mouseDelta_{0.0f, 0.0f};
  Vec2 mousePosition_{0.0f, 0.0f};
  bool mouseCaptured_{false};
};

} // namespace Eden
