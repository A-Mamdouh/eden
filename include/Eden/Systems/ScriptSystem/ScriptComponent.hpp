#pragma once

#include "Eden/Services/SceneService/Entity.hpp"
#include "ScriptBehaviour.hpp"

#include <memory>

namespace Eden::World {
class Scene;
}

namespace Eden::Scripting::Components {

/// Attaches a ScriptBehaviour to an entity; ScriptSystem drives it.
/// Requires its own entity and owning Scene at construction rather than
/// leaving them to be reconstructed per-frame from a registry view --
/// a ScriptComponent's lifecycle is inherently tied to both for as long
/// as it exists, so ScriptSystem just reads them back off the component
/// instead of rebuilding an Entity from the raw view handle every tick.
struct ScriptComponent {
  /// @param scene Stored as a pointer, not a reference: a reference
  ///        member would make ScriptComponent non-move-assignable,
  ///        which breaks EnTT's default swap-and-pop storage on removal.
  /// @param behaviour The behavior instance ScriptSystem drives; null
  ///        (the default) is a valid "no script yet" state that
  ///        ScriptSystem silently skips.
  ScriptComponent(World::Entity entity, World::Scene &scene,
                  std::unique_ptr<Scripting::ScriptBehaviour> behaviour = nullptr)
      : entity{entity}, scene{&scene}, behaviour{std::move(behaviour)} {
    if (this->behaviour) {
      this->behaviour->attach(entity, scene);
    }
  }

  World::Entity entity;
  World::Scene *scene;
  std::unique_ptr<Scripting::ScriptBehaviour> behaviour;
  /// Set by ScriptSystem after the first onStart() call. User code
  /// populating this component should leave it at the default.
  bool started{false};
};

} // namespace Eden::Scripting::Components
