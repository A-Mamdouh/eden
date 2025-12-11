#include "Eden/core/Configuration.hpp"

#include <utility>

namespace Eden {

std::unique_ptr<ConfigurationService> ConfigurationService::instance_ = nullptr;

ConfigurationService &ConfigurationService::getInstance() {
  if (instance_ == nullptr) {
    instance_ =
        std::unique_ptr<ConfigurationService>(new ConfigurationService());
  }
  return *instance_;
}

const EngineConfig &ConfigurationService::get() const noexcept {
  return config_;
}

EngineConfig &ConfigurationService::mutableConfig() noexcept { return config_; }

ConfigurationService::ListenerId
ConfigurationService::subscribe(Listener listener) {
  const auto id = nextListenerId_++;
  listeners_.emplace(id, std::move(listener));
  return id;
}

void ConfigurationService::unsubscribe(ListenerId id) { listeners_.erase(id); }

void ConfigurationService::apply(
    const std::function<void(EngineConfig &)> &mutator) {
  if (mutator) {
    mutator(config_);
    notify();
  }
}

void ConfigurationService::set(const EngineConfig &config) {
  config_ = config;
  notify();
}

void ConfigurationService::notify() const {
  for (const auto &entry : listeners_) {
    entry.second(config_);
  }
}

} // namespace Eden
