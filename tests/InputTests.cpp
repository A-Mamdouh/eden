#include <gtest/gtest.h>

#include "Eden/platform/Input.hpp"

TEST(InputSystem, KeyboardPressReleaseLifecycle)
{
    Eden::Input input;

    input.beginFrame();

    EXPECT_FALSE(input.isKeyDown(Eden::KeyCode::A));
    EXPECT_FALSE(input.wasKeyPressed(Eden::KeyCode::A));
    EXPECT_FALSE(input.wasKeyReleased(Eden::KeyCode::A));

    input.setKeyDown(Eden::KeyCode::A);

    EXPECT_TRUE(input.isKeyDown(Eden::KeyCode::A));
    EXPECT_TRUE(input.wasKeyPressed(Eden::KeyCode::A));
    EXPECT_FALSE(input.wasKeyReleased(Eden::KeyCode::A));

    input.beginFrame();

    EXPECT_TRUE(input.isKeyDown(Eden::KeyCode::A));
    EXPECT_FALSE(input.wasKeyPressed(Eden::KeyCode::A));
    EXPECT_FALSE(input.wasKeyReleased(Eden::KeyCode::A));

    input.setKeyUp(Eden::KeyCode::A);

    EXPECT_FALSE(input.isKeyDown(Eden::KeyCode::A));
    EXPECT_FALSE(input.wasKeyPressed(Eden::KeyCode::A));
    EXPECT_TRUE(input.wasKeyReleased(Eden::KeyCode::A));
}

TEST(InputSystem, MouseButtonsAndPosition)
{
    Eden::Input input;

    input.beginFrame();
    input.setMousePosition(10.0f, 20.0f);

    EXPECT_FLOAT_EQ(input.mouseX(), 10.0f);
    EXPECT_FLOAT_EQ(input.mouseY(), 20.0f);

    EXPECT_FALSE(input.isMouseButtonDown(Eden::MouseButton::Left));
    EXPECT_FALSE(input.wasMouseButtonPressed(Eden::MouseButton::Left));
    EXPECT_FALSE(input.wasMouseButtonReleased(Eden::MouseButton::Left));

    input.setMouseButtonDown(Eden::MouseButton::Left);

    EXPECT_TRUE(input.isMouseButtonDown(Eden::MouseButton::Left));
    EXPECT_TRUE(input.wasMouseButtonPressed(Eden::MouseButton::Left));
    EXPECT_FALSE(input.wasMouseButtonReleased(Eden::MouseButton::Left));

    input.beginFrame();

    EXPECT_TRUE(input.isMouseButtonDown(Eden::MouseButton::Left));
    EXPECT_FALSE(input.wasMouseButtonPressed(Eden::MouseButton::Left));
    EXPECT_FALSE(input.wasMouseButtonReleased(Eden::MouseButton::Left));

    input.setMouseButtonUp(Eden::MouseButton::Left);

    EXPECT_FALSE(input.isMouseButtonDown(Eden::MouseButton::Left));
    EXPECT_FALSE(input.wasMouseButtonPressed(Eden::MouseButton::Left));
    EXPECT_TRUE(input.wasMouseButtonReleased(Eden::MouseButton::Left));
}
