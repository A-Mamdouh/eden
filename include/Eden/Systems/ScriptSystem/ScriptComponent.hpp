#pragma once

#include "ScriptBehaviour.hpp"

#include <memory>

namespace Eden {

/// Attaches a ScriptBehaviour to an entity; ScriptSystem drives it.
struct ScriptComponent {
  /// The behavior instance ScriptSystem drives; null is a valid "no
  /// script yet" state that ScriptSystem silently skips.
  std::unique_ptr<ScriptBehaviour> behaviour;
  /// Set by ScriptSystem after the first onStart() call. User code
  /// populating this component should leave it at the default.
  bool started{false};
};

} // namespace Eden
