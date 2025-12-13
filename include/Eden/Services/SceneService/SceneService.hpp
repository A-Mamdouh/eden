#pragma once

#include "Eden/Services/IService.hpp"
#include "Scene.hpp"

#include <memory>

namespace Eden {

class SceneService : public IService {
public:
  std::string getName() override { return "Scene Service"; }

  void loadScene(std::unique_ptr<Scene> scene);

private:
  std::unique_ptr<Scene> activeScene_{nullptr};
};

} // namespace Eden