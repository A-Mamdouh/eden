#include "Eden/script/ScriptSystem.hpp"
#include "Eden/platform/Platform.hpp"
#include "Eden/scene/SceneManager.hpp"

namespace Eden
{

std::unique_ptr<ScriptSystem> ScriptSystem::instance_ = nullptr;

ScriptSystem& ScriptSystem::getInstance()
{
    if (instance_ == nullptr)
    {
        instance_ = std::unique_ptr<ScriptSystem>(new ScriptSystem());
    }
    return *instance_;
}

void ScriptSystem::init(const ConfigurationService&) {}

void ScriptSystem::update(float deltaTime)
{
    // Scene and input are provided by the Engine via the ScriptContext.
    if (!context_.scene || !context_.input)
    {
        return;
    }

    context_.renderer = nullptr;
    updateScripts(context_.scene->getRegistry(), deltaTime);
}

void ScriptSystem::setContext(const ScriptContext& context)
{
    context_ = context;
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

void ScriptSystem::shutdown()
{
    Registry* registry = nullptr;
    std::shared_ptr<Scene> fallbackScene{};
    if (context_.scene)
    {
        registry = &context_.scene->getRegistry();
    }
    else
    {
        fallbackScene = SceneManager::getInstance().getActiveScene();
        if (fallbackScene)
        {
            registry = &fallbackScene->getRegistry();
        }
    }

    if (registry)
    {
        auto view = registry->view<ScriptComponent>();
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
                Entity entity{*registry, entityId};
                component.behaviour->onEnd(entity);
                component.started = false;
            }
        }
    }

    context_ = {};
}

std::vector<ScriptBehaviour*> ScriptSystem::collectScripts(Registry& registry) const
{
    std::vector<ScriptBehaviour*> scripts;
    auto view = registry.view<ScriptComponent>();
    scripts.reserve(view.size());

    for (auto entityId : view)
    {
        auto& component = view.get<ScriptComponent>(entityId);
        if (component.behaviour)
        {
            scripts.push_back(component.behaviour.get());
        }
    }

    return scripts;
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
