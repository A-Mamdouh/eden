cmake_minimum_required(VERSION 3.21)

# Engine metadata
set(EDEN_ENGINE_METADATA_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/engine-meta.cmake")
if(NOT EXISTS "${EDEN_ENGINE_METADATA_FILE}")
    message(FATAL_ERROR "Engine metadata file not found: ${EDEN_ENGINE_METADATA_FILE}")
endif()
include("${EDEN_ENGINE_METADATA_FILE}")

project(
    Eden
    VERSION ${EDEN_ENGINE_VERSION}
    DESCRIPTION "${EDEN_ENGINE_DESCRIPTION}"
    LANGUAGES CXX C
)

# Keep the library available by default whether Eden is configured directly or
# brought into another project with add_subdirectory()/FetchContent. Developer
# targets default on only for a standalone Eden build.
set(_EDEN_BUILD_LIBRARY_DEFAULT ON)
if(DEFINED EDEN_BUILD_ENGINE AND NOT DEFINED EDEN_BUILD_LIBRARY)
    message(DEPRECATION
        "EDEN_BUILD_ENGINE is deprecated; use EDEN_BUILD_LIBRARY instead."
    )
    set(_EDEN_BUILD_LIBRARY_DEFAULT "${EDEN_BUILD_ENGINE}")
endif()

option(EDEN_BUILD_LIBRARY "Build the Eden engine library (requires the Vulkan SDK)" ${_EDEN_BUILD_LIBRARY_DEFAULT})

set(_EDEN_DEVELOPER_DEFAULT ${PROJECT_IS_TOP_LEVEL})
if(NOT EDEN_BUILD_LIBRARY)
    set(_EDEN_DEVELOPER_DEFAULT OFF)
endif()

option(EDEN_BUILD_DEMO "Build the Eden standalone demo executable" ${_EDEN_DEVELOPER_DEFAULT})
option(EDEN_BUILD_TESTS "Build the Eden test suite" ${_EDEN_DEVELOPER_DEFAULT})
option(EDEN_BUILD_DOCS "Build Eden documentation (requires Doxygen + Sphinx)" OFF)

unset(_EDEN_BUILD_LIBRARY_DEFAULT)
unset(_EDEN_DEVELOPER_DEFAULT)

if(NOT EDEN_BUILD_LIBRARY AND (EDEN_BUILD_DEMO OR EDEN_BUILD_TESTS))
    message(FATAL_ERROR
        "EDEN_BUILD_DEMO and EDEN_BUILD_TESTS require EDEN_BUILD_LIBRARY=ON."
    )
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Documentation only parses public headers, so a docs-only build deliberately
# avoids the graphics toolchain and all library/test dependencies.
if(EDEN_BUILD_LIBRARY)
    include(FetchContent)

    # FetchContent adds dependencies in this directory's variable scope, so a
    # parent project's BUILD_SHARED_LIBS value would otherwise decide how
    # those dependencies are built. Keep that override inside a function:
    # fetched dependencies see OFF, while the parent cache and directory scope
    # remain untouched. Packages supplied by the parent are used as-is.
    function(_eden_fetch_static DEPENDENCY)
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(${DEPENDENCY})
    endfunction()

    if(NOT TARGET spdlog::spdlog)
        find_package(spdlog CONFIG QUIET)
    endif()
    if(NOT TARGET spdlog::spdlog)
        set(EDEN_SPDLOG_VERSION v1.15.3 CACHE STRING "spdlog version fetched by Eden")
        FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG ${EDEN_SPDLOG_VERSION}
        )
        _eden_fetch_static(spdlog)
    endif()

    if(NOT TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static)
        find_package(SDL2 QUIET)
    endif()
    if(NOT TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static)
        set(EDEN_SDL2_VERSION release-2.30.10 CACHE STRING "SDL2 version fetched by Eden")
        FetchContent_Declare(
            SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG ${EDEN_SDL2_VERSION}
        )
        _eden_fetch_static(SDL2)
    endif()

    # Eden itself is a static library. Prefer SDL's static target when the SDL
    # package provides one, without changing the parent project's
    # BUILD_SHARED_LIBS policy. Fall back to the generic target for packages
    # that only provide a shared SDL build.
    if(TARGET SDL2::SDL2-static)
        set(EDEN_SDL2_TARGET SDL2::SDL2-static)
    elseif(TARGET SDL2::SDL2)
        set(EDEN_SDL2_TARGET SDL2::SDL2)
    else()
        message(FATAL_ERROR "SDL2 was found, but it did not provide a supported CMake target.")
    endif()

    # The Vulkan loader and glslc both come from the Vulkan SDK. Fetching
    # Vulkan-Hpp cannot replace either one, so report the real requirement.
    find_package(Vulkan REQUIRED)

    if(NOT TARGET EnTT::EnTT)
        find_package(EnTT CONFIG QUIET)
    endif()
    if(NOT TARGET EnTT::EnTT)
        set(EDEN_ENTT_VERSION v3.13.1 CACHE STRING "EnTT version fetched by Eden")
        FetchContent_Declare(
            EnTT
            GIT_REPOSITORY https://github.com/skypjack/entt.git
            GIT_TAG ${EDEN_ENTT_VERSION}
        )
        _eden_fetch_static(EnTT)
    endif()

    if(NOT TARGET glm::glm)
        find_package(glm CONFIG QUIET)
    endif()
    if(NOT TARGET glm::glm)
        set(EDEN_GLM_VERSION 1.0.1 CACHE STRING "glm version fetched by Eden")
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG ${EDEN_GLM_VERSION}
        )
        _eden_fetch_static(glm)
    endif()

    # Directory for generated public configuration headers.
    set(EDEN_ENGINE_CONFIG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
    file(MAKE_DIRECTORY "${EDEN_ENGINE_CONFIG_INCLUDE_DIR}/Eden")

    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/include/Eden/EdenConfig.hpp.in"
        "${EDEN_ENGINE_CONFIG_INCLUDE_DIR}/Eden/EdenConfig.hpp"
        @ONLY
    )

    find_program(GLSLC_EXECUTABLE glslc)
    if(NOT GLSLC_EXECUTABLE)
        message(FATAL_ERROR
            "glslc executable not found. Install the Vulkan SDK or ensure glslc is on PATH."
        )
    endif()

    # Keep generated assets inside Eden's own binary directory when Eden is a
    # subproject; never write into the parent project's CMAKE_BINARY_DIR.
    set(EDEN_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY "${EDEN_SHADER_OUTPUT_DIR}")

    set(EDEN_SHADER_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/shaders/mesh.vert"
        "${CMAKE_CURRENT_LIST_DIR}/shaders/mesh.frag"
    )

    set(EDEN_SHADER_OUTPUTS "")
    foreach(SHADER IN LISTS EDEN_SHADER_SOURCES)
        get_filename_component(SHADER_FILE_NAME "${SHADER}" NAME)
        set(SHADER_OUTPUT "${EDEN_SHADER_OUTPUT_DIR}/${SHADER_FILE_NAME}.spv")
        add_custom_command(
            OUTPUT "${SHADER_OUTPUT}"
            COMMAND "${GLSLC_EXECUTABLE}" "${SHADER}" -o "${SHADER_OUTPUT}"
            DEPENDS "${SHADER}"
            COMMENT "Compiling ${SHADER_FILE_NAME} to SPIR-V"
            VERBATIM
        )
        list(APPEND EDEN_SHADER_OUTPUTS "${SHADER_OUTPUT}")
    endforeach()

    add_custom_target(EdenShaders DEPENDS ${EDEN_SHADER_OUTPUTS})

    add_subdirectory(src)
    add_dependencies(Eden EdenShaders)

    if(EDEN_BUILD_DEMO)
        add_subdirectory(demo)
    endif()

    if(EDEN_BUILD_TESTS)
        if(NOT TARGET GTest::gtest_main)
            find_package(GTest QUIET)
        endif()
        if(NOT TARGET GTest::gtest_main)
            set(EDEN_GTEST_VERSION v1.17.0 CACHE STRING "GoogleTest version fetched by Eden")
            FetchContent_Declare(
                GTest
                GIT_REPOSITORY https://github.com/google/googletest.git
                GIT_TAG ${EDEN_GTEST_VERSION}
            )
            _eden_fetch_static(GTest)
        endif()

        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

if(EDEN_BUILD_DOCS)
    add_subdirectory(docs)
endif()
