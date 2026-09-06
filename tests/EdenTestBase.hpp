#pragma once

#include <Eden/Services/EventService/EventService.hpp>

#include <gtest/gtest.h>

#include <memory>

namespace EdenTest {

/// Common fixture: a live, self-initialized EventService. Every
/// IService/ISystem's init() publishes a Started event through one, so
/// anything under test needs a real (not null) one to avoid crashing on
/// a dangling weak_ptr.
class EdenTestBase : public ::testing::Test {
protected:
  void SetUp() override {
    eventService = std::make_shared<Eden::Services::EventService>();
    eventService->init(eventService);
  }

  void TearDown() override {
    try{
      auto defaultLogger = spdlog::default_logger();

      spdlog::drop_all();

      if (defaultLogger) {
          spdlog::set_default_logger(std::move(defaultLogger));
      }
    } catch(std::exception) {

    }
  }

  std::shared_ptr<Eden::Services::EventService> eventService;
};

} // namespace EdenTest
