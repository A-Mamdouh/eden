#pragma once

#include "Eden/Services/IService.hpp"
#include "Scene.hpp"

#include <memory>

namespace Eden {

/// Owns the active Scene and publishes SceneLoadedEvent on change. Not
/// currently constructed by Engine; Scene itself is presently a stub.
class SceneService : public IService {
public:
  std::string getName() override { return "Scene Service"; }

  /// Publishes Events::SceneLoadedEvent, then takes ownership of `scene`.
  /// @param scene New active scene; the previous one, if any, is destroyed.
  void loadScene(std::unique_ptr<Scene> scene);

private:
  void onInit() override {}
  std::unique_ptr<Scene> activeScene_{nullptr};
};

} // namespace Eden
