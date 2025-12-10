#include <Engine/Eden.hpp>
#include <spdlog/spdlog.h>

class PlayerControllerScript2 : public Eden::ScriptBehaviour
{
public:
    PlayerControllerScript2() = default;

    void onStart(Eden::Entity entity) override
    {
    }

    void onUpdate(Eden::Entity entity, float deltaTime) override
    {
        auto* input = context().input;
        if (!input)
        {
            return;
        }

        auto& rigidbody = entity.getComponent<Eden::Rigidbody2D>();
        auto& transform = entity.getComponent<Eden::Transform>();
        const float speed = 3e-3f;
        
        Eden::Vec2 delta{0.0f, 0.0f};
        if(!context().projectToScreen) {
            spdlog::info("Project to screen is null");
            return;
        }
        const auto& screenPosition = context().projectToScreen(transform.position);
        
        delta.x = (input->mouseX() - screenPosition.x) * speed;
        delta.y = (screenPosition.y - input->mouseY()) * speed;

        // Accumulate translation directly on the rigidbody; physics will apply it pre-step.
        if (delta.x != 0.0f || delta.y != 0.0f)
        {
            rigidbody.pendingTranslation += delta;
        }

        rigidbody.linearVelocity = {0.0f, 0.0f};
    }

    void onEnd(Eden::Entity entity) override
    {
        spdlog::info("PlayerControllerScript22 ended");
    }

private:
};
