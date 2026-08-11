#pragma once

#include "Eden/Services/SceneService/Entity.hpp"

namespace Eden {

/// Base class for per-entity behavior. Derive from this, override
/// onUpdate() (and optionally onStart()) to read/write the entity's own
/// components, and attach an instance via ScriptComponent -- ScriptSystem
/// drives it from there.
///
/// Deliberately minimal for now: no input or cross-entity access, since
/// there's no InputSystem yet and most scripts don't need one. A richer
/// context can be added to these hooks later without breaking existing
/// scripts, since it would just be an additional parameter.
class ScriptBehaviour {
public:
  virtual ~ScriptBehaviour() = default;

  /// Called once, the first time ScriptSystem ticks this entity.
  virtual void onStart(Entity /*entity*/) {}

  /// Called once per frame, after onStart() has already run.
  /// @param entity This behaviour's owning entity.
  /// @param dt Frame delta time in seconds.
  virtual void onUpdate(Entity entity, double dt) = 0;
};

} // namespace Eden
