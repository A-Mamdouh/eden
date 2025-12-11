#pragma once

#ifndef EDEN_ENGINE_SYSTEM_HPP
#define EDEN_ENGINE_SYSTEM_HPP

#include "Configuration.hpp"

namespace Eden {

class ISystem {
public:
  virtual ~ISystem() = default;

  virtual void init(const ConfigurationService &) {}
  virtual void update(float /*deltaTime*/) {}
  virtual void shutdown() {}
};

} // namespace Eden

#endif // EDEN_ENGINE_SYSTEM_HPP
