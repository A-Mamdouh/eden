#pragma once

namespace Eden::Input {

/// Vendor-neutral key identifiers InputSystem tracks -- deliberately a
/// small, hand-picked set (movement keys plus a couple of modifiers)
/// rather than every SDL scancode, so no SDL type has to appear in this
/// public header. Extend as new scripts need more keys.
enum class Key {
  W,
  A,
  S,
  D,
  Q,
  E,
  Space,
  LeftShift,
  LeftControl,
  Escape,
  Up,
  Down,
  Left,
  Right,
};

} // namespace Eden::Input
