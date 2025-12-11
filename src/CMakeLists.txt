add_library(Eden STATIC
    core/Application.cpp
    core/Configuration.cpp
    core/Engine.cpp
    core/EventSystem.cpp
    core/Log.cpp
    core/Scene.cpp
    core/SceneManager.cpp
    platform/Input.cpp
    platform/Platform.cpp
    platform/Window.cpp
    render/RenderSystem.cpp
    render/VulkanRenderer.cpp
    script/ScriptSystem.cpp
)

target_include_directories(
    Eden
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${EDEN_ENGINE_CONFIG_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/_deps/entt-src/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(Eden PUBLIC cxx_std_20)

target_compile_definitions(
    Eden
    PRIVATE
        EDEN_SHADER_DIR="${EDEN_SHADER_OUTPUT_DIR}"
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
