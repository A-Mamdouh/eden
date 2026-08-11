#pragma once

namespace Eden {

  /// Placeholder for the engine's scene representation. Currently holds
  /// no data or entities; SceneService owns instances of it, but nothing
  /// in RenderSystem walks a Scene yet -- see RenderSystem::buildDemoFrame.
  struct Scene{};

}