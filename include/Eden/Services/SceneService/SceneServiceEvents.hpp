#pragma once

#include "Eden/Events/IEvent.hpp"
#include "Scene.hpp"

namespace Eden::Events {
  /// Published by SceneService::loadScene(), before it takes ownership
  /// of the new scene.
  struct SceneLoadedEvent : IEvent {
    /// The newly loaded scene; SceneService takes ownership immediately
    /// after publishing this event.
    const World::Scene *scene;
  };
}