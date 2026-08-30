add_executable(Eden_Demo main.cpp DemoMeshes.cpp DemoScene.cpp)

target_include_directories(Eden_Demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Eden_Demo PRIVATE Eden::Eden spdlog::spdlog)
target_compile_features(Eden_Demo PRIVATE cxx_std_20)
target_compile_definitions(Eden_Demo PRIVATE EDEN_DEMO_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

add_dependencies(Eden_Demo EdenShaders)
