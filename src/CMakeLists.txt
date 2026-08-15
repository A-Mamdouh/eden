add_library(Eden STATIC
    Core/Engine.cpp
    Services/IService.cpp
    Services/ClockService/ClockService.cpp
    Services/ConfigService/ConfigService.cpp
    Services/JobService/JobService.cpp
    Services/SceneService/Scene.cpp
    Services/SceneService/SceneService.cpp
    Systems/ISystem.cpp
    Systems/TransformSystem.cpp
    Systems/InputSystem/InputSystem.cpp
    Systems/InputSystem/InputState.cpp
    Systems/ScriptSystem/ScriptSystem.cpp
    Systems/RenderSystem/RenderSystem.cpp
    Systems/RenderSystem/NullRenderer.cpp
    Systems/RenderSystem/GltfLoader.cpp
    Systems/RenderSystem/Bounds.cpp
    Systems/RenderSystem/Frustum.cpp
    Systems/RenderSystem/Vulkan/VulkanRenderer.cpp
)

target_include_directories(
    Eden
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${EDEN_ENGINE_CONFIG_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/_deps/entt-src/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

target_compile_features(Eden PUBLIC cxx_std_20)

target_compile_definitions(
    Eden
    PRIVATE
        EDEN_SHADER_DIR="${EDEN_SHADER_OUTPUT_DIR}"
    PUBLIC
        # Vulkan's clip volume expects NDC z in [0, 1], not OpenGL's [-1, 1];
        # every glm projection matrix (Camera::projectionMatrix(), any future
        # ortho/perspective use) must be built under this convention. PUBLIC
        # so any TU including glm via a public Eden header (e.g. Math.hpp)
        # sees the same macro before glm's headers are first parsed.
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)

target_link_libraries(Eden PUBLIC glm::glm)

target_link_libraries(
    Eden
    PRIVATE
        spdlog::spdlog
        SDL2::SDL2
        Vulkan::Vulkan
)

target_compile_definitions(Eden PRIVATE EDEN_ENABLE_VULKAN)
