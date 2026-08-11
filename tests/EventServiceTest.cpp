#include <Eden/Events/IEvent.hpp>
#include <Eden/Services/EventService/EventService.hpp>

#include <gtest/gtest.h>

namespace {

struct TestEvent : Eden::IEvent {
  int value{0};
};

struct OtherEvent : Eden::IEvent {
  int value{0};
};

} // namespace

TEST(EventServiceTest, PublishInvokesSubscribedListener) {
  Eden::EventService events;
  int received = -1;

  events.subscribe<TestEvent>([&](const TestEvent &event) { received = event.value; });
  events.publish(TestEvent{.value = 42});

  EXPECT_EQ(received, 42);
}

TEST(EventServiceTest, PublishWithNoListenersDoesNothing) {
  Eden::EventService events;
  EXPECT_NO_THROW(events.publish(TestEvent{.value = 1}));
}

TEST(EventServiceTest, MultipleListenersAllReceiveTheEvent) {
  Eden::EventService events;
  int firstCount = 0;
  int secondCount = 0;

  events.subscribe<TestEvent>([&](const TestEvent &) { ++firstCount; });
  events.subscribe<TestEvent>([&](const TestEvent &) { ++secondCount; });
  events.publish(TestEvent{});

  EXPECT_EQ(firstCount, 1);
  EXPECT_EQ(secondCount, 1);
}

TEST(EventServiceTest, UnsubscribeStopsDelivery) {
  Eden::EventService events;
  int count = 0;

  const auto id = events.subscribe<TestEvent>([&](const TestEvent &) { ++count; });
  events.publish(TestEvent{});
  events.unsubscribe<TestEvent>(id);
  events.publish(TestEvent{});

  EXPECT_EQ(count, 1);
}

TEST(EventServiceTest, DifferentEventTypesDoNotCrossFire) {
  Eden::EventService events;
  bool testEventFired = false;
  bool otherEventFired = false;

  events.subscribe<TestEvent>([&](const TestEvent &) { testEventFired = true; });
  events.subscribe<OtherEvent>([&](const OtherEvent &) { otherEventFired = true; });

  events.publish(OtherEvent{});

  EXPECT_FALSE(testEventFired);
  EXPECT_TRUE(otherEventFired);
}
