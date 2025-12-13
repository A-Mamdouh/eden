#pragma once

#include <Eden/script/Script.hpp>
#include <Eden/core/Application.hpp>

class GameControls : public Eden::ScriptBehaviour {

    void onUpdate(Eden::Entity entity, float deltaTime) override {
        if(context().input->isKeyDown(Eden::KeyCode::Escape)) {
            context().application->exit();
        }
    }
};