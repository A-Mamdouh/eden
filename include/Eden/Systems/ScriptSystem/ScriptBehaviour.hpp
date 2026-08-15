#pragma once

#include "Eden/Services/SceneService/Entity.hpp"

namespace Eden {

class InputState;

/// Base class for per-entity behavior. Derive from this, override
/// onUpdate() (and optionally onStart()) to read/write the entity's own
/// components, and attach an instance via ScriptComponent -- ScriptSystem
/// drives it from there.
///
/// Deliberately minimal beyond that: a script sees its own entity, dt,
/// and InputState (for keyboard/mouse queries) -- no cross-entity
/// access, no Renderer/EventService, and no InputSystem either, since
/// InputState is the whole query/request surface a script needs. Non-const
/// InputState& because scripts legitimately mutate it too (e.g. releasing
/// mouse capture), not just query it.
class ScriptBehaviour {
public:
  virtual ~ScriptBehaviour() = default;

  /// Called once, the first time ScriptSystem ticks this entity.
  virtual void onStart(Entity /*entity*/, InputState & /*input*/) {}

  /// Called once per frame, after onStart() has already run.
  /// @param entity This behaviour's owning entity.
  /// @param dt Frame delta time in seconds.
  /// @param input This frame's keyboard/mouse state.
  virtual void onUpdate(Entity entity, double dt, InputState &input) = 0;
};

} // namespace Eden
