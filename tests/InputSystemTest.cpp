#include <Eden/Systems/InputSystem/InputSystem.hpp>

#include <gtest/gtest.h>

// Deliberately never calls InputSystem::init(): SDL_GetKeyboardState()/
// SDL_PumpEvents()/SDL_GetRelativeMouseState() are all safe to call
// before SDL_Init() (they read/no-op against SDL's own static state
// rather than requiring a live subsystem), which is what makes this
// testable without a window -- same reasoning as RenderSystemMaterialTest.

TEST(InputSystemTest, NoKeysAreDownBeforeAnyUpdate) {
  Eden::InputSystem input;

  EXPECT_FALSE(input.state().isKeyDown(Eden::Key::W));
  EXPECT_FALSE(input.state().isKeyPressed(Eden::Key::W));
}

TEST(InputSystemTest, UpdateWithoutARealEventLoopDoesNotCrash) {
  Eden::InputSystem input;

  EXPECT_NO_THROW(input.update(0.016));

  EXPECT_FALSE(input.state().isKeyDown(Eden::Key::W));
  EXPECT_EQ(input.state().mouseDelta(), Eden::Vec2(0.0f, 0.0f));
}

TEST(InputSystemTest, MouseCaptureFlagRoundTrips) {
  Eden::InputSystem input;

  EXPECT_FALSE(input.state().mouseCaptured());
  input.state().setMouseCaptured(true);
  EXPECT_TRUE(input.state().mouseCaptured());
  input.state().setMouseCaptured(false);
  EXPECT_FALSE(input.state().mouseCaptured());
}
