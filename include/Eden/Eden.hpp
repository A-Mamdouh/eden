#pragma once

#include "Eden/Core/Engine.hpp"
#include "Eden/Services/ConfigService/Config.hpp"
#include "Eden/Services/SceneService/Components.hpp"
#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Systems/InputSystem/InputSystem.hpp"
#include "Eden/Systems/InputSystem/Key.hpp"
#include "Eden/Systems/RenderSystem/Model.hpp"
#include "Eden/Systems/RenderSystem/RenderableComponent.hpp"
#include "Eden/Systems/ScriptSystem/ScriptBehaviour.hpp"
#include "Eden/Systems/ScriptSystem/ScriptComponent.hpp"

/// Single umbrella header for applications embedding Eden: pulls in
/// Engine, the config types needed to construct one, Scene plus the
/// components needed to populate one for Engine::loadScene(),
/// ScriptBehaviour for attaching per-entity behavior, and InputSystem/Key
/// for reading keyboard/mouse state from within one.
namespace Eden {
/// Config type an application fills out and passes to Engine's constructor.
using AppConfig = Config::ApplicationConfig;
} // namespace Eden

