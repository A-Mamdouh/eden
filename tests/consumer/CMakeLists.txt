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

add_executable(EdenConsumerSmoke main.cpp)
target_link_libraries(EdenConsumerSmoke PRIVATE Eden::Eden)
target_compile_features(EdenConsumerSmoke PRIVATE cxx_std_20)
