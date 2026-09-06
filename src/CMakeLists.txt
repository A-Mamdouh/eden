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

add_library(Eden::Eden ALIAS Eden)

target_include_directories(
    Eden
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${EDEN_ENGINE_CONFIG_INCLUDE_DIR}>
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

target_link_libraries(
    Eden
    PUBLIC
        EnTT::EnTT
        glm::glm
        spdlog::spdlog
)

target_link_libraries(
    Eden
    PRIVATE
        ${EDEN_SDL2_TARGET}
        Vulkan::Vulkan
)

target_compile_definitions(Eden PRIVATE EDEN_ENABLE_VULKAN)
