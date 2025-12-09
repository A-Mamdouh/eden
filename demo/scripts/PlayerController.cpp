#include "Engine/Eden.hpp"
#include <spdlog/spdlog.h>

class PlayerControllerScript : public Eden::ScriptBehaviour
{
public:
    PlayerControllerScript() = default;

    void onStart(Eden::Entity entity) override
    {
        (void)entity;
    }

    void onUpdate(Eden::Entity entity, float deltaTime) override
    {
        auto* input = context().input;
        if (!input)
        {
            return;
        }

        auto& transform = entity.getComponent<Eden::Transform>();
        const float speed = 1.5f;

        if (input->isKeyDown(Eden::KeyCode::A))
        {
            transform.position.x -= speed * deltaTime;
        }
        if (input->isKeyDown(Eden::KeyCode::D))
        {
            transform.position.x += speed * deltaTime;
        }
        if (input->isKeyDown(Eden::KeyCode::W))
        {
            transform.position.y += speed * deltaTime;
        }
        if (input->isKeyDown(Eden::KeyCode::S))
        {
            transform.position.y -= speed * deltaTime;
        }
    }

    void onEnd(Eden::Entity entity) override
    {
        (void)entity;
        spdlog::info("PlayerControllerScript ended");
    }

private:
};
