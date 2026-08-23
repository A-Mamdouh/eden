#pragma once

#include "Eden/Events/IEvent.hpp"
#include "InputState.hpp"

namespace Eden::Events {

/// Published by InputSystem::update(), once per frame after it refreshes
/// its InputState from SDL. The subscriber-facing replacement for
/// wiring InputSystem into another System's constructor: ScriptSystem
/// (or anything else that needs live input) subscribes once in its own
/// onInit() and caches the pointer, rather than depending on
/// InputSystem directly.
struct InputStateUpdatedEvent : public IEvent {
  /// Non-owning pointer to InputSystem's own InputState; valid for
  /// InputSystem's lifetime, which outlives this event. Non-const since
  /// subscribers (e.g. a script via ScriptBehaviour) legitimately mutate
  /// it too, e.g. setMouseCaptured().
  InputState *state;
};

} // namespace Eden::Events
