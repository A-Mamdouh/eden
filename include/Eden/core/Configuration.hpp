#pragma once

#ifndef EDEN_ENGINE_CONFIGURATION_HPP
#define EDEN_ENGINE_CONFIGURATION_HPP

#include "Config.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>

namespace Eden {

class ConfigurationService {
public:
  static ConfigurationService &getInstance();

  ConfigurationService(const ConfigurationService &) = delete;
  ConfigurationService &operator=(const ConfigurationService &) = delete;
  ConfigurationService(ConfigurationService &&) noexcept = delete;
  ConfigurationService &operator=(ConfigurationService &&) noexcept = delete;

  const EngineConfig &get() const noexcept;
  EngineConfig &mutableConfig() noexcept;

  using ListenerId = std::size_t;
  using Listener = std::function<void(const EngineConfig &)>;

  ListenerId subscribe(Listener listener);
  void unsubscribe(ListenerId id);

  void apply(const std::function<void(EngineConfig &)> &mutator);
  void set(const EngineConfig &config);

private:
  ConfigurationService() = default;
  static std::unique_ptr<ConfigurationService> instance_;

  void notify() const;

  EngineConfig config_{};
  std::unordered_map<ListenerId, Listener> listeners_{};
  ListenerId nextListenerId_{1};
};

} // namespace Eden

#endif // EDEN_ENGINE_CONFIGURATION_HPP
