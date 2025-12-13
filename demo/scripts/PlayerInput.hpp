#pragma once

#include <Eden/ecs/TransformComponent.hpp>
#include <Eden/render/Renderer.hpp>
#include <Eden/script/Script.hpp>

class PlayerInput : public Eden::ScriptBehaviour {

    void onUpdate(Eden::Entity entity, float deltaTime) override {
        Eden::Vec2 delta{0.0f, 0.0f};
        if(context().input->isKeyDown(Eden::KeyCode::A)) {
            delta.x -= 1;
        }
        if(context().input->isKeyDown(Eden::KeyCode::D)) {
            delta.x += 1;
        }
        if(context().input->isKeyDown(Eden::KeyCode::W)) {
            delta.y += 1;
        }
        if(context().input->isKeyDown(Eden::KeyCode::S)) {
            delta.y -= 1;
        }
        delta = glm::normalize(delta) * speed;
        if(abs(delta.x) > 1e-3 || abs(delta.y) > 1e-3) {
            auto &transform = entity.getComponent<Eden::Transform>();
            transform.position.x += delta.x;
            transform.position.y += delta.y;
        }
        totalTime += deltaTime;
    }
    
    void onRender(Eden::Entity entity, Eden::Renderer& renderer) override {
        const Eden::Color color{
            .r = static_cast<float>(sin(totalTime) / 2.0f + 0.5f),
            .g = static_cast<float>(sin(totalTime) / 2.0f + 0.5f),
            .b = static_cast<float>(cos(sin(totalTime)) / 2.0f + 0.5f),
            .a = 1.0f
        };
        renderer.clear(color);
    }

    private:
        float totalTime = 0.0f;
        float speed = 0.01;
};