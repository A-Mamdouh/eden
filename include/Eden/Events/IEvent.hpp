#pragma once

#include <type_traits>

namespace Eden {

/// Base tag for all event payload structs published through EventService.
struct IEvent {};

/// Satisfied by any type derived from IEvent; constrains
/// EventService::subscribe/unsubscribe/publish.
template <typename T>
concept is_event_type = std::is_base_of_v<IEvent, T>;

} // namespace Eden