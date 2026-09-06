#include <Eden/Systems/InputSystem/InputSystem.hpp>

#include "EdenTestBase.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace Eden::Systems;
using namespace Eden::Input;

class InputSystemTest : public EdenTest::EdenTestBase {};

namespace {

class FailingSystem : public Eden::ISystem {
public:
  std::string getName() override { return "Failing Test System"; }
  void update(double) override {}
  void shutdown() override {}

private:
  void onInit() override { throw std::runtime_error{"Expected initialization failure"}; }
};

} // namespace

TEST_F(InputSystemTest, NoKeysAreDownBeforeAnyUpdate) {
  InputSystem input;

  EXPECT_FALSE(input.isInitialized());
  EXPECT_FALSE(input.state().isKeyDown(Key::W));
  EXPECT_FALSE(input.state().isKeyPressed(Key::W));
}

TEST_F(InputSystemTest, UpdateRequiresInitAndWorksWithoutARealEventLoop) {
  InputSystem input;

  EXPECT_THROW(input.update(0.016), std::logic_error);

  input.init(eventService);

  EXPECT_TRUE(input.isInitialized());
  EXPECT_THROW(input.init(eventService), std::logic_error);
  EXPECT_NO_THROW(input.update(0.016));

  EXPECT_FALSE(input.state().isKeyDown(Key::W));
  EXPECT_EQ(input.state().mouseDelta(), Eden::Vec2(0.0f, 0.0f));

  FailingSystem failingSystem;
  EXPECT_THROW(failingSystem.init(eventService), std::runtime_error);
  EXPECT_FALSE(failingSystem.isInitialized());
  EXPECT_EQ(spdlog::get(failingSystem.getName()), nullptr);
}

TEST_F(InputSystemTest, MouseCaptureFlagRoundTrips) {
  InputSystem input;

  EXPECT_FALSE(input.state().mouseCaptured());
  input.state().setMouseCaptured(true);
  EXPECT_TRUE(input.state().mouseCaptured());
  input.state().setMouseCaptured(false);
  EXPECT_FALSE(input.state().mouseCaptured());
}
