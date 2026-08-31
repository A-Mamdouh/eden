cmake_minimum_required(VERSION 3.21)

project(EdenConsumerSmokeTest LANGUAGES CXX)

# This exercises the same subproject behavior used by FetchContent without
# requiring a network clone of the repository under test.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." eden)

if(NOT TARGET Eden::Eden)
    message(FATAL_ERROR "Eden::Eden was not created for a dependency build")
endif()

if(EDEN_BUILD_DEMO OR EDEN_BUILD_TESTS)
    message(FATAL_ERROR "Eden developer targets must default off as a subproject")
endif()

if(TARGET Eden_Demo OR TARGET EdenTests OR TARGET GTest::gtest_main)
    message(FATAL_ERROR "Eden configured a developer-only target for a consumer")
endif()

# The parent deliberately configures this smoke test with BUILD_SHARED_LIBS=ON.
# Dependencies built by Eden must still be static, so the consumer does not
# unexpectedly acquire runtime DLLs/shared objects from Eden's FetchContent
# implementation details.
foreach(DEPENDENCY_TARGET IN ITEMS Eden spdlog glm SDL2 SDL2-static)
    if(TARGET ${DEPENDENCY_TARGET})
        get_target_property(DEPENDENCY_TYPE ${DEPENDENCY_TARGET} TYPE)
        if(DEPENDENCY_TYPE STREQUAL "SHARED_LIBRARY" OR
           DEPENDENCY_TYPE STREQUAL "MODULE_LIBRARY")
            message(FATAL_ERROR
                "${DEPENDENCY_TARGET} was built as ${DEPENDENCY_TYPE}; "
                "Eden must isolate fetched dependencies from BUILD_SHARED_LIBS."
            )
        endif()
    endif()
endforeach()

add_executable(EdenConsumerSmoke main.cpp)
target_link_libraries(EdenConsumerSmoke PRIVATE Eden::Eden)
target_compile_features(EdenConsumerSmoke PRIVATE cxx_std_20)
