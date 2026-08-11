cmake_minimum_required(VERSION 3.21)

# Detect if this is the top-level CMake project.
set(EDEN_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(EDEN_IS_TOP_LEVEL ON)
endif()

# Engine metadata
set(EDEN_ENGINE_METADATA_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/engine-meta.cmake")
if(NOT EXISTS "${EDEN_ENGINE_METADATA_FILE}")
    message(FATAL_ERROR "Engine metadata file not found: ${EDEN_ENGINE_METADATA_FILE}")
endif()
include("${EDEN_ENGINE_METADATA_FILE}")

option(EDEN_BUILD_ENGINE "Build the Eden engine library, demo, and tests (requires the Vulkan SDK)" ON)
option(EDEN_BUILD_DEMO "Build Eden standalone demo executable" ${EDEN_IS_TOP_LEVEL})
option(EDEN_BUILD_DOCS "Build Eden documentation (requires Doxygen + Sphinx)" OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Define project using the configured metadata
project(
    Eden
    VERSION ${EDEN_ENGINE_VERSION}
    DESCRIPTION "${EDEN_ENGINE_DESCRIPTION}"
    LANGUAGES CXX C
)

set(BUILD_SHARED_LIBS FALSE)

# Everything in this block is about building/testing the engine itself and
# needs the Vulkan SDK (for glslc) plus every FetchContent dependency below.
# Documentation only parses header text, so EDEN_BUILD_DOCS deliberately
# doesn't require any of it -- that's what keeps a docs-only CI job fast
# and independent of engine build health.
if (EDEN_BUILD_ENGINE)

find_package(Git REQUIRED)

find_package(spdlog QUIET)
if (NOT spdlog_FOUND)
    if (NOT DEFINED SPDLOG_VERSION)
        set(SPDLOG_VERSION v1.15.3)
    endif()
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG ${SPDLOG_VERSION}
    )
    FetchContent_MakeAvailable(spdlog)
endif()

find_package(SDL2 QUIET)
if (NOT SDL2_FOUND)
    if (NOT DEFINED SDL2_VERSION)
        # Reasonably recent SDL2 release tag; can be overridden by the user.
        set(SDL2_VERSION release-2.30.10)
    endif()
    include(FetchContent)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG ${SDL2_VERSION}
    )
    FetchContent_MakeAvailable(SDL2)
endif()

find_package(Vulkan QUIET)
if (NOT Vulkan_FOUND)
    if (NOT DEFINED VULKAN_VERSION)
        set(VULKAN_VERSION v1.4.328)
    endif()
    include(FetchContent)
    FetchContent_Declare(
        Vulkan
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Hpp.git
        GIT_TAG ${VULKAN_VERSION}
    )
    FetchContent_MakeAvailable(Vulkan)
endif()

find_package(entt QUIET)
if (NOT entt_FOUND)
    if (NOT DEFINED ENTT_VERSION)
        set(ENTT_VERSION v3.13.1)
    endif()

    include(FetchContent)
    FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG ${ENTT_VERSION}
    )
    FetchContent_MakeAvailable(entt)
endif()

find_package(glm QUIET)
if (NOT glm_FOUND)
    if (NOT DEFINED GLM_VERSION)
        set(GLM_VERSION 1.0.1)
    endif()

    include(FetchContent)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG ${GLM_VERSION}
    )
    FetchContent_MakeAvailable(glm)
endif()

# find_package(assimp QUIET)
# if (NOT assimp_FOUND)
#     if (NOT DEFINED ASSIMP_VERSION)
#         set(ASSIMP_VERSION v6.0.2)
#     endif()
#     include(FetchContent)
#     FetchContent_Declare(
#         assimp
#         GIT_REPOSITORY https://github.com/assimp/assimp.git
#         GIT_TAG ${ASSIMP_VERSION}
#     )
#     FetchContent_MakeAvailable(assimp)
# endif()

# find_package(nlohmann_json QUIET)
# if (NOT nlohmann_json_FOUND)
#     if (NOT DEFINED NLOHMANN_JSON_VERSION)
#         set(NLOHMANN_JSON_VERSION v3.12.0)
#     endif()
#     include(FetchContent)
#     FetchContent_Declare(
#         nlohmann_json
#         GIT_REPOSITORY https://github.com/nlohmann/json.git
#         GIT_TAG ${NLOHMANN_JSON_VERSION}
#     )
#     FetchContent_MakeAvailable(nlohmann_json)
# endif()

find_package(GTest QUIET)
if (NOT GTest_FOUND)
    if (NOT DEFINED GTEST_VERSION)
        set(GTEST_VERSION v1.17.0)
    endif()
    include(FetchContent)
    FetchContent_Declare(
        GTest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG ${GTEST_VERSION}
    )
    FetchContent_MakeAvailable(GTest)
endif()

# Directory for generated config headers
set(EDEN_ENGINE_CONFIG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
file(MAKE_DIRECTORY "${EDEN_ENGINE_CONFIG_INCLUDE_DIR}")

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/include/Eden/EdenConfig.hpp.in
    ${EDEN_ENGINE_CONFIG_INCLUDE_DIR}/Eden/EdenConfig.hpp
    @ONLY
)

find_program(GLSLC_EXECUTABLE glslc)
if (NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc executable not found. Install the Vulkan SDK or ensure glslc is on PATH.")
endif()

set(EDEN_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${EDEN_SHADER_OUTPUT_DIR}")

set(EDEN_SHADER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/shaders/triangle.vert
    ${CMAKE_CURRENT_LIST_DIR}/shaders/triangle.frag
)

set(EDEN_SHADER_OUTPUTS "")
foreach(SHADER ${EDEN_SHADER_SOURCES})
    get_filename_component(SHADER_FILE_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${EDEN_SHADER_OUTPUT_DIR}/${SHADER_FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_FILE_NAME} to SPIR-V"
        VERBATIM
    )
    list(APPEND EDEN_SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(EdenShaders DEPENDS ${EDEN_SHADER_OUTPUTS})

enable_testing()

add_subdirectory(src)
if (EDEN_BUILD_DEMO)
    add_subdirectory(demo)
endif()
add_subdirectory(tests)

add_dependencies(Eden EdenShaders)

endif() # EDEN_BUILD_ENGINE

if (EDEN_BUILD_DOCS)
    add_subdirectory(docs)
endif()
