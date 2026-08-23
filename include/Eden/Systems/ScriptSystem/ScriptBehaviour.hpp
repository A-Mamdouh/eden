#pragma once

#include "Eden/Services/SceneService/Entity.hpp"

namespace Eden::Input {
class InputState;
}

namespace Eden::World {
class Scene;
}

namespace Eden::Scripting::Components {
struct ScriptComponent;
} // namespace Eden::Scripting::Components

namespace Eden::Scripting {

/// Base class for per-entity behavior. Derive from this, override
/// onUpdate() (and optionally onStart()) to read/write the entity's own
/// components, and attach an instance via ScriptComponent -- ScriptSystem
/// drives it from there.
///
/// entity() and scene() are set once, by ScriptComponent's constructor,
/// before either hook ever runs -- not re-passed as parameters, since a
/// ScriptBehaviour belongs to exactly one entity/Scene for its entire
/// life, same reasoning ScriptComponent itself already follows. Scene is
/// mutable -- createEntity()/destroyEntity() are both fair game, see
/// Scene's own doc for the one destroyEntity() caveat. InputState (for
/// keyboard/mouse queries) still comes in as a parameter, since it's
/// shared, frame-changing state, not something owned per-script. Beyond
/// that: no cross-entity access beyond what Scene itself exposes, no
/// Renderer/EventService. Non-const InputState& because scripts
/// legitimately mutate it too (e.g. releasing mouse capture), not just
/// query it.
class ScriptBehaviour {
public:
  virtual ~ScriptBehaviour() = default;

  /// Called once, the first time ScriptSystem ticks this entity.
  virtual void onStart(Input::InputState & /*input*/) {}

  /// Called once per frame, after onStart() has already run.
  /// @param dt Frame delta time in seconds.
  /// @param input This frame's keyboard/mouse state.
  virtual void onUpdate(double dt, Input::InputState &input) = 0;

  /// @return This behaviour's owning entity.
  World::Entity entity() const { return entity_; }
  /// @return This entity's owning Scene.
  World::Scene &scene() const { return *scene_; }

private:
  friend struct Components::ScriptComponent;
  /// Called exactly once, by ScriptComponent's constructor, before
  /// onStart()/onUpdate() ever run.
  void attach(World::Entity entity, World::Scene &scene) {
    entity_ = entity;
    scene_ = &scene;
  }

  World::Entity entity_;
  World::Scene *scene_{nullptr};
};

} // namespace Eden::Scripting
