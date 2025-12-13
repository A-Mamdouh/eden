#pragma once

#include <type_traits>

namespace Eden {

struct IEvent {};

template <typename T>
concept is_event_type = std::is_base_of_v<IEvent, T>;

} // namespace Eden