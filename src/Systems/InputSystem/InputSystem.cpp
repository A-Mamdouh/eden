#include "Eden/Systems/InputSystem/InputSystem.hpp"

#include "Eden/Services/EventService/EventService.hpp"
#include "Eden/Systems/InputSystem/InputSystemEvents.hpp"
#include "spdlog/spdlog.h"

#include <SDL.h>

namespace Eden::Systems
{

  void InputSystem::update(double /*dt*/)
  {
    int numKeys = 0;
    const Uint8 *keyboardState = SDL_GetKeyboardState(&numKeys);

    if (state_.previousKeys_.size() != static_cast<std::size_t>(numKeys))
    {
      state_.previousKeys_.assign(static_cast<std::size_t>(numKeys), false);
      state_.currentKeys_.assign(static_cast<std::size_t>(numKeys), false);
    }
    state_.previousKeys_ = state_.currentKeys_;

    SDL_PumpEvents();

    for (int i = 0; i < numKeys; ++i)
    {
      state_.currentKeys_[static_cast<std::size_t>(i)] = keyboardState[i] != 0;
    }

    // A subscriber (e.g. a script) may have called state_.setMouseCaptured()
    // against last frame's state; apply that request to SDL now, before
    // reading this frame's motion, so mouseDelta()/mousePosition() below
    // reflect the mode they're actually being read in.
    const bool requestedCapture = state_.mouseCaptured();
    if (SDL_GetRelativeMouseMode() != (requestedCapture ? SDL_TRUE : SDL_FALSE))
    {
      SDL_SetRelativeMouseMode(requestedCapture ? SDL_TRUE : SDL_FALSE);
    }

    int deltaX = 0;
    int deltaY = 0;
    SDL_GetRelativeMouseState(&deltaX, &deltaY);
    state_.mouseDelta_ = Vec2{static_cast<float>(deltaX), static_cast<float>(deltaY)};

    int posX = 0;
    int posY = 0;
    SDL_GetMouseState(&posX, &posY);
    state_.mousePosition_ = Vec2{static_cast<float>(posX), static_cast<float>(posY)};

    // Guarded, unlike other systems' publish() calls: InputSystem is
    // deliberately usable (see InputSystemTest) without init() ever having
    // been called, since none of its SDL calls above need it either.
    const auto maybeEventService = getEventService();
    if (maybeEventService.has_value())
    {
      const auto eventService = maybeEventService.value();
      eventService->publish<Events::InputStateUpdatedEvent>(Events::InputStateUpdatedEvent{.state = &state_});
    }
    else
    {
      logger_->warn("Failed to send state updated event. Event service is not available");
    }
  }

} // namespace Eden::Systems
