#include <Eden/Services/ClockService/ClockService.hpp>

#include "EdenTestBase.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

using namespace Eden::Services;

class ClockServiceTest : public EdenTest::EdenTestBase {};

namespace {

class FailingService : public Eden::IService {
public:
  std::string getName() override { return "Failing Test Service"; }

private:
  void onInit() override { throw std::runtime_error{"Expected initialization failure"}; }
};

} // namespace

TEST_F(ClockServiceTest, TickReturnsNonNegativeDelta) {
  ClockService clock;

  EXPECT_FALSE(clock.isInitialized());
  EXPECT_THROW(clock.tick(), std::logic_error);

  clock.init(eventService);

  EXPECT_TRUE(clock.isInitialized());
  const auto registeredClockLogger = spdlog::get(clock.getName());
  EXPECT_THROW(clock.init(eventService), std::logic_error);
  EXPECT_TRUE(clock.isInitialized());
  EXPECT_EQ(spdlog::get(clock.getName()), registeredClockLogger);

  EXPECT_GE(clock.tick(), 0.0);

  FailingService failingService;
  EXPECT_THROW(failingService.init(eventService), std::runtime_error);
  EXPECT_FALSE(failingService.isInitialized());
  EXPECT_EQ(spdlog::get(failingService.getName()), nullptr);
}

TEST_F(ClockServiceTest, PausedTickReturnsZero) {
  ClockService clock;
  clock.init(eventService);
  clock.setPaused(true);

  EXPECT_EQ(clock.tick(), 0.0);
  EXPECT_TRUE(clock.isPaused());
}

TEST_F(ClockServiceTest, TimeScaleZeroFreezesFrameDt) {
  ClockService clock;
  clock.init(eventService);
  clock.setTimeScale(0.0);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  EXPECT_DOUBLE_EQ(clock.tick(), 0.0);
}

TEST_F(ClockServiceTest, FrameDtIsClampedToMaxFrameDt) {
  Eden::Config::Clock::ClockConfig config{};
  config.maxFrameDt = 0.05;
  ClockService clock{config};
  clock.init(eventService);

  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  EXPECT_LE(clock.tick(), config.maxFrameDt + 0.01);
}

TEST_F(ClockServiceTest, RealTimeAccumulatesEvenWhilePaused) {
  ClockService clock;
  clock.init(eventService);
  clock.setPaused(true);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  clock.tick();

  EXPECT_GT(clock.realTime(), 0.0);
}

TEST_F(ClockServiceTest, FixedDtReturnsConfiguredValue) {
  Eden::Config::Clock::ClockConfig config{};
  config.fixedDt = 0.125;
  ClockService clock{config};

  EXPECT_DOUBLE_EQ(clock.fixedDt(), 0.125);
}

TEST_F(ClockServiceTest, ConsumeFixedStepFalseBeforeEnoughTimeAccumulates) {
  Eden::Config::Clock::ClockConfig config{};
  config.fixedDt = 10.0;
  ClockService clock{config};
  clock.init(eventService);

  clock.tick();

  EXPECT_FALSE(clock.consumeFixedStep());
}

TEST_F(ClockServiceTest, ConsumeFixedStepTrueOnceEnoughTimeAccumulates) {
  Eden::Config::Clock::ClockConfig config{};
  config.fixedDt = 0.01;
  ClockService clock{config};
  clock.init(eventService);

  clock.tick();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  clock.tick();

  EXPECT_TRUE(clock.consumeFixedStep());
}
