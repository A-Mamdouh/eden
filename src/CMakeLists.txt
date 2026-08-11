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
    Systems/RenderSystem/RenderSystem.cpp
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
