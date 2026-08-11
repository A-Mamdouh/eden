#pragma once

#include "Eden/Core/Engine.hpp"
#include "Eden/Services/ConfigService/Config.hpp"
#include "Eden/Services/SceneService/Components.hpp"
#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Systems/RenderSystem/RenderableComponent.hpp"

/// Single umbrella header for applications embedding Eden: pulls in
/// Engine, the config types needed to construct one, and Scene plus the
/// components needed to populate one for Engine::loadScene().
namespace Eden {
/// Config type an application fills out and passes to Engine's constructor.
using AppConfig = Config::ApplicationConfig;
} // namespace Eden

