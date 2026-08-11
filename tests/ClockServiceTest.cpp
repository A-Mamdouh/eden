#include <Eden/Services/ClockService/ClockService.hpp>

#include "EdenTestBase.hpp"

#include <chrono>
#include <thread>

class ClockServiceTest : public EdenTest::EdenTestBase {};

TEST_F(ClockServiceTest, TickReturnsNonNegativeDelta) {
  Eden::ClockService clock;
  clock.init(eventService);

  EXPECT_GE(clock.tick(), 0.0);
}

TEST_F(ClockServiceTest, PausedTickReturnsZero) {
  Eden::ClockService clock;
  clock.init(eventService);
  clock.setPaused(true);

  EXPECT_EQ(clock.tick(), 0.0);
  EXPECT_TRUE(clock.isPaused());
}

TEST_F(ClockServiceTest, TimeScaleZeroFreezesFrameDt) {
  Eden::ClockService clock;
  clock.init(eventService);
  clock.setTimeScale(0.0);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  EXPECT_DOUBLE_EQ(clock.tick(), 0.0);
}

TEST_F(ClockServiceTest, FrameDtIsClampedToMaxFrameDt) {
  Eden::Config::ClockConfig config{};
  config.maxFrameDt = 0.05;
  Eden::ClockService clock{config};
  clock.init(eventService);

  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  EXPECT_LE(clock.tick(), config.maxFrameDt + 0.01);
}

TEST_F(ClockServiceTest, RealTimeAccumulatesEvenWhilePaused) {
  Eden::ClockService clock;
  clock.init(eventService);
  clock.setPaused(true);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  clock.tick();

  EXPECT_GT(clock.realTime(), 0.0);
}

TEST_F(ClockServiceTest, FixedDtReturnsConfiguredValue) {
  Eden::Config::ClockConfig config{};
  config.fixedDt = 0.125;
  Eden::ClockService clock{config};

  EXPECT_DOUBLE_EQ(clock.fixedDt(), 0.125);
}

TEST_F(ClockServiceTest, ConsumeFixedStepFalseBeforeEnoughTimeAccumulates) {
  Eden::Config::ClockConfig config{};
  config.fixedDt = 10.0;
  Eden::ClockService clock{config};
  clock.init(eventService);

  clock.tick();

  EXPECT_FALSE(clock.consumeFixedStep());
}

TEST_F(ClockServiceTest, ConsumeFixedStepTrueOnceEnoughTimeAccumulates) {
  Eden::Config::ClockConfig config{};
  config.fixedDt = 0.01;
  Eden::ClockService clock{config};
  clock.init(eventService);

  clock.tick();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  clock.tick();

  EXPECT_TRUE(clock.consumeFixedStep());
}
