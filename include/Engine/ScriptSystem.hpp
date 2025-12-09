//#pragma once

#ifndef EDEN_ENGINE_SCRIPT_SYSTEM_HPP
#define EDEN_ENGINE_SCRIPT_SYSTEM_HPP

#include "Engine/Script.hpp"

namespace Eden
{

class ScriptSystem
{
public:
    ScriptSystem() = default;

    void setContext(const ScriptContext& context);
    void updateScripts(Registry& registry, float deltaTime);
    void renderScripts(Registry& registry, Renderer& renderer);
    void shutdown(Registry& registry);

private:
    void ensureStarted(Entity entity, ScriptComponent& component);
    void applyContext(ScriptComponent& component);

    ScriptContext context_{};
};

} // namespace Eden

#endif // EDEN_ENGINE_SCRIPT_SYSTEM_HPP
