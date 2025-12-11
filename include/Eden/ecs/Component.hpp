#pragma once

#ifndef EDEN_ENGINE_COMPONENTS_HPP
#define EDEN_ENGINE_COMPONENTS_HPP
#include <type_traits>

namespace Eden {
struct Component {};

template <typename T> concept is_component = std::is_base_of_v<Component, T>;

} // namespace Eden

#endif // EDEN_ENGINE_COMPONENTS_HPP
