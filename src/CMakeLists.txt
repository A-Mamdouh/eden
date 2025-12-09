add_library(Eden STATIC
    Application.cpp
    Engine.cpp
    Input.cpp
    Log.cpp
    ScriptSystem.cpp
    VulkanRenderer.cpp
    Window.cpp
)

target_include_directories(
    Eden
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${EDEN_ENGINE_CONFIG_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/_deps/entt-src/src
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
)

if (Vulkan_FOUND)
    target_link_libraries(Eden PRIVATE Vulkan::Vulkan)
    target_compile_definitions(Eden PRIVATE EDEN_ENABLE_VULKAN)
endif()
