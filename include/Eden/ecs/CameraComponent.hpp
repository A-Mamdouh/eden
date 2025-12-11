#pragma once

#ifndef EDEN_ENGINE_CAMERA_HPP
#define EDEN_ENGINE_CAMERA_HPP

#include "Eden/core/Math.hpp"
#include "Component.hpp"

namespace Eden {


    struct Camera : public Component
    {
        Mat4 view{Mat4(1.0f)};
        Mat4 projection{Mat4(1.0f)};
        bool active;
    };

} // namespace Eden

#endif // EDEN_ENGINE_CAMERA_HPP
