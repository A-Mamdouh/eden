#include "Eden/scene/SceneManager.hpp"
#include <memory>

namespace Eden
{

std::unique_ptr<SceneManager> SceneManager::instance_ = nullptr;

SceneManager& SceneManager::getInstance()
{
    if (instance_ == nullptr)
    {
        instance_ = std::unique_ptr<SceneManager>(new SceneManager());
    }
    return *instance_;
}

SceneManager::SceneManager()
{
    scene_ = nullptr;
}

void SceneManager::setScene(std::shared_ptr<Scene> scene)
{
    detachScene();
    scene_ = scene;
    attachScene();
}

std::shared_ptr<Scene> SceneManager::getActiveScene() noexcept
{
    return scene_;
}

const std::shared_ptr<Scene> SceneManager::getActiveScene() const noexcept
{
    return scene_;
}

bool SceneManager::hasActiveScene() const noexcept
{
    return scene_ != nullptr;
}

void SceneManager::detachScene()
{
    if (scene_)
    {
        scene_->unload();
    }
}

SceneManager::~SceneManager() {
    detachScene();
}

void SceneManager::attachScene()
{
    if (!scene_)
    {
        return;
    }

    scene_->load();
}

} // namespace Eden
