#include "Eden/platform/Input.hpp"

namespace Eden
{

namespace
{
    template <typename Enum>
    constexpr std::size_t toIndex(Enum value)
    {
        return static_cast<std::size_t>(value);
    }
} // namespace

Input::Input() = default;

void Input::beginFrame()
{
    keysPressed_.fill(false);
    keysReleased_.fill(false);
    mousePressed_.fill(false);
    mouseReleased_.fill(false);
}

void Input::setKeyDown(KeyCode key)
{
    const auto idx = toIndex(key);
    if (!keysDown_[idx])
    {
        keysDown_[idx] = true;
        keysPressed_[idx] = true;
    }
}

void Input::setKeyUp(KeyCode key)
{
    const auto idx = toIndex(key);
    if (keysDown_[idx])
    {
        keysDown_[idx] = false;
        keysReleased_[idx] = true;
    }
}

void Input::setMouseButtonDown(MouseButton button)
{
    const auto idx = toIndex(button);
    if (!mouseDown_[idx])
    {
        mouseDown_[idx] = true;
        mousePressed_[idx] = true;
    }
}

void Input::setMouseButtonUp(MouseButton button)
{
    const auto idx = toIndex(button);
    if (mouseDown_[idx])
    {
        mouseDown_[idx] = false;
        mouseReleased_[idx] = true;
    }
}

void Input::setMousePosition(float x, float y)
{
    mouseX_ = x;
    mouseY_ = y;
}

bool Input::isKeyDown(KeyCode key) const
{
    return keysDown_[toIndex(key)];
}

bool Input::wasKeyPressed(KeyCode key) const
{
    return keysPressed_[toIndex(key)];
}

bool Input::wasKeyReleased(KeyCode key) const
{
    return keysReleased_[toIndex(key)];
}

bool Input::isMouseButtonDown(MouseButton button) const
{
    return mouseDown_[toIndex(button)];
}

bool Input::wasMouseButtonPressed(MouseButton button) const
{
    return mousePressed_[toIndex(button)];
}

bool Input::wasMouseButtonReleased(MouseButton button) const
{
    return mouseReleased_[toIndex(button)];
}

float Input::mouseX() const noexcept
{
    return mouseX_;
}

float Input::mouseY() const noexcept
{
    return mouseY_;
}

} // namespace Eden
