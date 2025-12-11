#include "Eden/core/EventSystem.hpp"

namespace Eden
{

std::unique_ptr<EventSystem> EventSystem::instance_ = nullptr;

EventSystem& EventSystem::getInstance()
{
    if (instance_ == nullptr)
    {
        instance_ = std::unique_ptr<EventSystem>(new EventSystem());
    }
    return *instance_;
}

void EventSystem::beginFrame()
{
    input_.beginFrame();
}

void EventSystem::update(float)
{
    beginFrame();
}

Input& EventSystem::getInput() noexcept
{
    return input_;
}

const Input& EventSystem::getInput() const noexcept
{
    return input_;
}

} // namespace Eden
