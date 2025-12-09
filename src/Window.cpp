#include "Engine/Window.hpp"

#include "Engine/Input.hpp"
#include "Log.hpp"

#include <SDL.h>

namespace Eden
{

namespace
{
    KeyCode fromSdlScancode(SDL_Scancode scancode)
    {
        switch (scancode)
        {
        case SDL_SCANCODE_A: return KeyCode::A;
        case SDL_SCANCODE_B: return KeyCode::B;
        case SDL_SCANCODE_C: return KeyCode::C;
        case SDL_SCANCODE_D: return KeyCode::D;
        case SDL_SCANCODE_E: return KeyCode::E;
        case SDL_SCANCODE_F: return KeyCode::F;
        case SDL_SCANCODE_G: return KeyCode::G;
        case SDL_SCANCODE_H: return KeyCode::H;
        case SDL_SCANCODE_I: return KeyCode::I;
        case SDL_SCANCODE_J: return KeyCode::J;
        case SDL_SCANCODE_K: return KeyCode::K;
        case SDL_SCANCODE_L: return KeyCode::L;
        case SDL_SCANCODE_M: return KeyCode::M;
        case SDL_SCANCODE_N: return KeyCode::N;
        case SDL_SCANCODE_O: return KeyCode::O;
        case SDL_SCANCODE_P: return KeyCode::P;
        case SDL_SCANCODE_Q: return KeyCode::Q;
        case SDL_SCANCODE_R: return KeyCode::R;
        case SDL_SCANCODE_S: return KeyCode::S;
        case SDL_SCANCODE_T: return KeyCode::T;
        case SDL_SCANCODE_U: return KeyCode::U;
        case SDL_SCANCODE_V: return KeyCode::V;
        case SDL_SCANCODE_W: return KeyCode::W;
        case SDL_SCANCODE_X: return KeyCode::X;
        case SDL_SCANCODE_Y: return KeyCode::Y;
        case SDL_SCANCODE_Z: return KeyCode::Z;

        case SDL_SCANCODE_0: return KeyCode::Num0;
        case SDL_SCANCODE_1: return KeyCode::Num1;
        case SDL_SCANCODE_2: return KeyCode::Num2;
        case SDL_SCANCODE_3: return KeyCode::Num3;
        case SDL_SCANCODE_4: return KeyCode::Num4;
        case SDL_SCANCODE_5: return KeyCode::Num5;
        case SDL_SCANCODE_6: return KeyCode::Num6;
        case SDL_SCANCODE_7: return KeyCode::Num7;
        case SDL_SCANCODE_8: return KeyCode::Num8;
        case SDL_SCANCODE_9: return KeyCode::Num9;

        case SDL_SCANCODE_ESCAPE: return KeyCode::Escape;
        case SDL_SCANCODE_SPACE: return KeyCode::Space;
        case SDL_SCANCODE_RETURN: return KeyCode::Enter;
        case SDL_SCANCODE_TAB: return KeyCode::Tab;
        case SDL_SCANCODE_BACKSPACE: return KeyCode::Backspace;

        case SDL_SCANCODE_LSHIFT: return KeyCode::LeftShift;
        case SDL_SCANCODE_RSHIFT: return KeyCode::RightShift;
        case SDL_SCANCODE_LCTRL: return KeyCode::LeftControl;
        case SDL_SCANCODE_RCTRL: return KeyCode::RightControl;
        case SDL_SCANCODE_LALT: return KeyCode::LeftAlt;
        case SDL_SCANCODE_RALT: return KeyCode::RightAlt;

        case SDL_SCANCODE_LEFT: return KeyCode::Left;
        case SDL_SCANCODE_RIGHT: return KeyCode::Right;
        case SDL_SCANCODE_UP: return KeyCode::Up;
        case SDL_SCANCODE_DOWN: return KeyCode::Down;

        default:
            return KeyCode::Unknown;
        }
    }

    MouseButton fromSdlMouseButton(Uint8 button)
    {
        switch (button)
        {
        case SDL_BUTTON_LEFT: return MouseButton::Left;
        case SDL_BUTTON_RIGHT: return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        default: return MouseButton::Left;
        }
    }

    class SdlWindow final : public Window
    {
    public:
        SdlWindow(const WindowConfig& config, Input& input)
            : input_(input)
        {
            Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
            if (config.resizable)
            {
                flags |= SDL_WINDOW_RESIZABLE;
            }

            window_ = SDL_CreateWindow(
                config.title.c_str(),
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                static_cast<int>(config.width),
                static_cast<int>(config.height),
                flags);

            if (!window_)
            {
                EDEN_CORE_ERROR("Failed to create SDL window: {}", SDL_GetError());
                shouldClose_ = true;
                return;
            }

            width_ = config.width;
            height_ = config.height;

            EDEN_CORE_INFO(
                "Created SDL window {}x{} ({})",
                width_,
                height_,
                config.title);
        }

        ~SdlWindow() override
        {
            if (window_)
            {
                SDL_DestroyWindow(window_);
                window_ = nullptr;
            }

            SDL_QuitSubSystem(SDL_INIT_VIDEO);
        }

        void pollEvents() override
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_QUIT:
                    shouldClose_ = true;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        shouldClose_ = true;
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        width_ = static_cast<unsigned int>(event.window.data1);
                        height_ = static_cast<unsigned int>(event.window.data2);
                    }
                    break;
                case SDL_KEYDOWN:
                    if (!event.key.repeat)
                    {
                        if (auto key = fromSdlScancode(event.key.keysym.scancode);
                            key != KeyCode::Unknown)
                        {
                            input_.setKeyDown(key);
                        }
                    }
                    break;
                case SDL_KEYUP:
                    if (auto key = fromSdlScancode(event.key.keysym.scancode);
                        key != KeyCode::Unknown)
                    {
                        input_.setKeyUp(key);
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    input_.setMouseButtonDown(fromSdlMouseButton(event.button.button));
                    break;
                case SDL_MOUSEBUTTONUP:
                    input_.setMouseButtonUp(fromSdlMouseButton(event.button.button));
                    break;
                case SDL_MOUSEMOTION:
                    input_.setMousePosition(
                        static_cast<float>(event.motion.x),
                        static_cast<float>(event.motion.y));
                    break;
                default:
                    break;
                }
            }
        }

        bool shouldClose() const noexcept override
        {
            return shouldClose_;
        }

        unsigned int width() const noexcept override
        {
            return width_;
        }

        unsigned int height() const noexcept override
        {
            return height_;
        }

        void* nativeHandle() noexcept override
        {
            return window_;
        }

        void setTitle(const std::string& title) override
        {
            if (window_)
            {
                SDL_SetWindowTitle(window_, title.c_str());
            }
        }

    private:
        SDL_Window* window_{nullptr};
        unsigned int width_{0};
        unsigned int height_{0};
        bool shouldClose_{false};
        Input& input_;
    };
} // namespace

std::unique_ptr<Window> createWindow(const WindowConfig& config, Input& input)
{
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
    {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
        {
            EDEN_CORE_ERROR("Failed to initialize SDL video subsystem: {}", SDL_GetError());
            return nullptr;
        }
    }

    return std::make_unique<SdlWindow>(config, input);
}

} // namespace Eden
