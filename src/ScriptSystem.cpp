#include "Engine/ScriptSystem.hpp"

namespace Eden
{

void ScriptSystem::setContext(const ScriptContext& context)
{
    context_ = context;
    if (context_.projectToScreen == nullptr && context_.scene)
    {
        context_.projectToScreen = [scene = context_.scene](const Vec3& world) {
            return scene->projectToScreen(world);
        };
    }
}

void ScriptSystem::updateScripts(Registry& registry, float deltaTime)
{
    context_.renderer = nullptr;

    auto view = registry.view<ScriptComponent>();
    for (auto entityId : view)
    {
        auto& component = view.get<ScriptComponent>(entityId);
        if (!component.behaviour)
        {
            continue;
        }

        applyContext(component);
        Entity entity{registry, entityId};
        ensureStarted(entity, component);
        component.behaviour->onUpdate(entity, deltaTime);
    }
}

void ScriptSystem::renderScripts(Registry& registry, Renderer& renderer)
{
    context_.renderer = &renderer;

    auto view = registry.view<ScriptComponent>();
    for (auto entityId : view)
    {
        auto& component = view.get<ScriptComponent>(entityId);
        if (!component.behaviour)
        {
            continue;
        }

        applyContext(component);
        Entity entity{registry, entityId};
        ensureStarted(entity, component);
        component.behaviour->onRender(entity, renderer);
    }

    context_.renderer = nullptr;
}

void ScriptSystem::shutdown(Registry& registry)
{
    auto view = registry.view<ScriptComponent>();
    for (auto entityId : view)
    {
        auto& component = view.get<ScriptComponent>(entityId);
        if (!component.behaviour)
        {
            continue;
        }

        if (component.started)
        {
            applyContext(component);
            Entity entity{registry, entityId};
            component.behaviour->onEnd(entity);
            component.started = false;
        }
    }
}

void ScriptSystem::ensureStarted(Entity entity, ScriptComponent& component)
{
    if (!component.started)
    {
        applyContext(component);
        component.behaviour->onStart(entity);
        component.started = true;
    }
}

void ScriptSystem::applyContext(ScriptComponent& component)
{
    if (component.behaviour)
    {
        component.behaviour->setContext(context_);
    }
}

} // namespace Eden
