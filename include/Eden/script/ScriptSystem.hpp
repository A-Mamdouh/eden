#pragma once

#include "Eden/core/System.hpp"
#ifndef EDEN_ENGINE_SCRIPT_SYSTEM_HPP
#define EDEN_ENGINE_SCRIPT_SYSTEM_HPP

#include "Script.hpp"

#include <memory>
#include <vector>

namespace Eden
{

class ScriptSystem : public ISystem
{
public:
    static ScriptSystem& getInstance();

    ScriptSystem(const ScriptSystem&) = delete;
    ScriptSystem& operator=(const ScriptSystem&) = delete;

    ScriptSystem(ScriptSystem&&) noexcept = delete;
    ScriptSystem& operator=(ScriptSystem&&) noexcept = delete;

    void init(const ConfigurationService& config) override;
    void update(float deltaTime) override;
    void shutdown() override;

    void setContext(const ScriptContext& context);
    void updateScripts(Registry& registry, float deltaTime);
    void renderScripts(Registry& registry, Renderer& renderer);

    std::vector<ScriptBehaviour*> collectScripts(Registry& registry) const;

private:
    ScriptSystem() = default;

    void ensureStarted(Entity entity, ScriptComponent& component);
    void applyContext(ScriptComponent& component);

    static std::unique_ptr<ScriptSystem> instance_;
    ScriptContext context_{};
};

} // namespace Eden

#endif // EDEN_ENGINE_SCRIPT_SYSTEM_HPP
