//#pragma once

#ifndef EDEN_ENGINE_INPUT_HPP
#define EDEN_ENGINE_INPUT_HPP

#include <array>
#include <cstdint>

namespace Eden
{

// Keyboard keys used by the engine API.
// This list can be extended over time as needed.
enum class KeyCode : std::uint16_t
{
    Unknown = 0,

    A, B, C, D, E, F, G,
    H, I, J, K, L, M, N,
    O, P, Q, R, S, T, U,
    V, W, X, Y, Z,

    Num0, Num1, Num2, Num3, Num4,
    Num5, Num6, Num7, Num8, Num9,

    Escape,
    Space,
    Enter,
    Tab,
    Backspace,
    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,

    Left, Right, Up, Down,

    Count
};

enum class MouseButton : std::uint8_t
{
    Left = 0,
    Right,
    Middle,

    Count
};

/**
 * Per-frame input state queried by gameplay code.
 *
 * This class is backend-agnostic. Platform layers (e.g. SDL) translate
 * OS events into calls to the setters below.
 */
class Input
{
public:
    Input();

    void beginFrame();

    // Keyboard setters called by the platform layer.
    void setKeyDown(KeyCode key);
    void setKeyUp(KeyCode key);

    // Mouse setters called by the platform layer.
    void setMouseButtonDown(MouseButton button);
    void setMouseButtonUp(MouseButton button);
    void setMousePosition(float x, float y);

    // Keyboard queries for gameplay.
    [[nodiscard]] bool isKeyDown(KeyCode key) const;
    [[nodiscard]] bool wasKeyPressed(KeyCode key) const;
    [[nodiscard]] bool wasKeyReleased(KeyCode key) const;

    // Mouse queries for gameplay.
    [[nodiscard]] bool isMouseButtonDown(MouseButton button) const;
    [[nodiscard]] bool wasMouseButtonPressed(MouseButton button) const;
    [[nodiscard]] bool wasMouseButtonReleased(MouseButton button) const;
    [[nodiscard]] float mouseX() const noexcept;
    [[nodiscard]] float mouseY() const noexcept;

private:
    static constexpr std::size_t keyCount =
        static_cast<std::size_t>(KeyCode::Count);
    static constexpr std::size_t mouseButtonCount =
        static_cast<std::size_t>(MouseButton::Count);

    std::array<bool, keyCount> keysDown_{};
    std::array<bool, keyCount> keysPressed_{};
    std::array<bool, keyCount> keysReleased_{};

    std::array<bool, mouseButtonCount> mouseDown_{};
    std::array<bool, mouseButtonCount> mousePressed_{};
    std::array<bool, mouseButtonCount> mouseReleased_{};

    float mouseX_{0.0f};
    float mouseY_{0.0f};
};

} // namespace Eden

#endif // EDEN_ENGINE_INPUT_HPP
