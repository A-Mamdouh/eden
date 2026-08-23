#pragma once
#include "Config.hpp"
#include "Eden/Services/IService.hpp"

namespace Eden::Services {

  /// Owns the live ApplicationConfig and publishes ConfigUpdatedEvent
  /// when it changes.
  class ConfigService : public IService {

    public:
      /// @param config Initial configuration, copied into this service.
      ConfigService(const Config::ApplicationConfig &config): config_{config} {}
      std::string getName() override { return "Config Service"; }
      /// Publishes Events::ConfigUpdatedEvent, then replaces the config.
      /// @param newconfig Configuration to become the new live value.
      void update(const Config::ApplicationConfig& newconfig);
      /// @return The current live configuration.
      const Config::ApplicationConfig& get() const { return config_; }

    private:
      void onInit() override {}
      Config::ApplicationConfig config_;
  };

} // namespace Eden::Services
