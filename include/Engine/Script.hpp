//#pragma once

#ifndef EDEN_ENGINE_SCRIPT_HPP
#define EDEN_ENGINE_SCRIPT_HPP

#include <memory>

#include "Engine/Entity.hpp"
#include "Engine/Renderer.hpp"
#include "Engine/Input.hpp"
#include "Engine/Scene.hpp"

namespace Eden
{

/**
 * Base class for gameplay scripts attached to ECS entities.
 *
 * Engines can derive custom behaviour classes from this interface
 * and attach them to entities via ScriptComponent.
 */
struct ScriptContext
{
    Input* input{nullptr};
    Renderer* renderer{nullptr};
    Scene* scene{nullptr};
};

class ScriptBehaviour
{
public:
    virtual ~ScriptBehaviour() = default;

    virtual void onStart(Entity entity) { (void)entity; }
    virtual void onUpdate(Entity entity, float deltaTime) { (void)entity; (void)deltaTime; }
    virtual void onRender(Entity entity, Renderer& renderer) { (void)entity; (void)renderer; }
    virtual void onEnd(Entity entity) { (void)entity; }

    void setContext(const ScriptContext& context) { context_ = context; }

protected:
    const ScriptContext& context() const noexcept { return context_; }
    ScriptContext& context() noexcept { return context_; }

private:
    ScriptContext context_{};
};

struct ScriptComponent
{
    std::unique_ptr<ScriptBehaviour> behaviour{};
    bool started{false};
};

template <typename Behaviour, typename... Args>
ScriptComponent makeScriptComponent(Args&&... args)
{
    ScriptComponent component{};
    component.behaviour = std::make_unique<Behaviour>(static_cast<Args&&>(args)...);
    return component;
}

} // namespace Eden

#endif // EDEN_ENGINE_SCRIPT_HPP
