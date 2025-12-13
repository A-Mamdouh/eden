#pragma once

#include "Eden/Events/IEvent.hpp"
#include "Scene.hpp"

namespace Eden::Events {
  struct SceneLoadedEvent : IEvent {
    const Scene *scene;
  };
}