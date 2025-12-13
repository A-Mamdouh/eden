#pragma once
#include "Config.hpp"
#include "Eden/Services/IService.hpp"

namespace Eden {

  class ConfigService : public IService {

    public:
      ConfigService(const Config::ApplicationConfig &config): config_{config} {}
      std::string getName() override { return "Config Service"; }
      void update(const Config::ApplicationConfig& newconfig);

    private:
      Config::ApplicationConfig config_;
  };

} // namespace Eden